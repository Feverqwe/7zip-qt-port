#!/usr/bin/env python3
"""Opt-in native SMB interoperability check; exports synthetic bytes only."""
import argparse
import base64
from collections import deque
from concurrent.futures import ThreadPoolExecutor
import errno
import mmap
import os
from pathlib import Path
import queue
import re
import shutil
import subprocess
import sys
import tempfile
import threading
import unicodedata


def command(arguments, **kwargs):
    result = subprocess.run(arguments, text=True, capture_output=True, timeout=45, **kwargs)
    if result.returncode:
        raise RuntimeError(f'{arguments[0]} exited {result.returncode}: '
                           f'{result.stdout.strip()} {result.stderr.strip()}')
    return result


def powershell(script):
    wrapped = ("$ErrorActionPreference='Stop'\ntry {\n" + script +
               "\n} catch { [Console]::Error.WriteLine($_.Exception.Message); exit 1 }\n")
    encoded = base64.b64encode(wrapped.encode('utf-16le')).decode('ascii')
    return command(['powershell.exe', '-NoProfile', '-NonInteractive', '-EncodedCommand', encoded])


def expected(offset, size):
    return bytes((offset + index) % 251 for index in range(size))


def check_contents(root):
    names = {unicodedata.normalize('NFC', name): name for name in os.listdir(root)}
    sizes = {'hello.txt': 65536, 'Файл с пробелом.bin': 262144,
             'large.bin': (5 << 30) + 123}
    assert set(names) == set(sizes), names

    def read_file(name):
        path = root / names[name]
        size = sizes[name]
        actual = path.stat().st_size
        assert actual == size, f'{name}: size {actual}, expected {size}'
        with path.open('rb', buffering=0) as stream:
            offsets = [0, 17, size - 4096]
            if name == 'large.bin':
                offsets.append((1 << 32) + 100003)
            for offset in offsets:
                stream.seek(offset)
                assert stream.read(4096) == expected(offset, 4096), (name, offset)
            stream.seek(size)
            eof = stream.read(4096)
            assert eof == b'', f'{name}: EOF returned {len(eof)} bytes'
        return name

    with ThreadPoolExecutor(max_workers=3) as workers:
        list(workers.map(read_file, sizes))
    offset = (1 << 32) + 2 * mmap.ALLOCATIONGRANULARITY
    with (root / names['large.bin']).open('rb') as stream:
        with mmap.mmap(stream.fileno(), 4096, access=mmap.ACCESS_READ, offset=offset) as mapped:
            assert mapped[:4096] == expected(offset, 4096)

    source = root / names['hello.txt']
    attempts = {
        'write': lambda: os.open(source, os.O_WRONLY),
        'create': lambda: os.open(root / 'new.bin', os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600),
        'delete': lambda: os.unlink(source),
        'rename': lambda: os.rename(source, root / 'renamed.txt'),
        'metadata': lambda: os.utime(source, (1700000000, 1700000000)),
    }
    for name, attempt in attempts.items():
        try:
            result = attempt()
        except OSError as error:
            assert error.errno in (errno.EROFS, errno.EACCES, errno.EPERM), (name, error)
        else:
            if isinstance(result, int):
                os.close(result)
            raise AssertionError(f'Native client allowed {name}')
    print('PASS: directory, Unicode/spaces, parallel ranges, >4GiB, EOF, mmap, read-only')


def mount(port, temporary, kernel_probe, global_mapping):
    if sys.platform == 'win32':
        drive = next((chr(letter) + ':' for letter in range(ord('Z'), ord('D') - 1, -1)
                      if not Path(chr(letter) + ':\\').exists()), None)
        if drive is None:
            raise RuntimeError('No unused drive letter for the isolated fixture')
        existing = powershell("$maps = @(Get-SmbMapping) + @(Get-SmbGlobalMapping)\n"
                              "@($maps | Where-Object RemotePath -eq '\\\\127.0.0.1\\Sevenzip').Count\n")
        if existing.stdout.strip() != '0':
            raise RuntimeError('An existing mapping owns the fixture UNC; preserving it')
        if global_mapping:
            powershell("$secret = ConvertTo-SecureString 'LocalProbeOnly427' -AsPlainText -Force\n"
                       "$credential = [PSCredential]::new('SevenzipLocal\\szprobe', $secret)\n"
                       "$owner = [Security.Principal.WindowsIdentity]::GetCurrent().Name\n"
                       f"New-SmbGlobalMapping -LocalPath '{drive}' -RemotePath '\\\\127.0.0.1\\Sevenzip' "
                       f"-Credential $credential -TcpPort {port} -FullAccess @($owner) "
                       "-RequireIntegrity $true -Persistent $false | Out-Null\n")
        else:
            powershell(f"New-SmbMapping -LocalPath '{drive}' -RemotePath '\\\\127.0.0.1\\Sevenzip' "
                       f"-UserName 'SevenzipLocal\\szprobe' -Password 'LocalProbeOnly427' -TcpPort {port} "
                       "-RequireIntegrity $true -Persistent $false | Out-Null\n")
        remove = 'Remove-SmbGlobalMapping' if global_mapping else 'Remove-SmbMapping'
        return Path(drive + '\\'), lambda: powershell(
            f"{remove} -LocalPath '{drive}' -Force -Confirm:$false\n")
    target = temporary / 'volume'
    target.mkdir()
    if sys.platform == 'darwin':
        command(['/sbin/mount_smbfs', '-o', 'ro,nobrowse,nopassprompt,forcenewsession',
                 f'//szprobe:LocalProbeOnly427@127.0.0.1:{port}/Sevenzip', str(target)])
        return target, lambda: command(['/sbin/umount', str(target)])
    if not sys.platform.startswith('linux'):
        raise RuntimeError('No native SMB probe for this OS')
    helper = shutil.which('mount.cifs')
    if not helper and not kernel_probe:
        raise RuntimeError('Install system CIFS support and cifs-utils, then retry; '
                           'Sevenzip does not supply a filesystem driver')
    privilege = [] if os.geteuid() == 0 else ['sudo', '-n']
    credentials = temporary / 'credentials'
    credentials.write_text('username=szprobe\npassword=LocalProbeOnly427\n')
    credentials.chmod(0o600)
    options = (f'ro,nosuid,nodev,port={port},vers=2.1,sec=ntlmsspi,nounix,'
               f'uid={os.getuid()},gid={os.getgid()},file_mode=0400,dir_mode=0500,cache=none,noserverino')
    if helper:
        command([*privilege, helper, '//127.0.0.1/Sevenzip', str(target), '-o',
                 options + ',credentials=' + str(credentials)])
    else:
        # Explicit research-only path: util-linux calls the existing kernel CIFS
        # client. The product still reports its missing cifs-utils prerequisite.
        print('Kernel-only diagnostic; mount.cifs is absent (no packages installed).')
        command([*privilege, 'mount', '-i', '-t', 'cifs', '//127.0.0.1/Sevenzip', str(target),
                 '-o', options + ',addr=127.0.0.1,unc=//127.0.0.1/Sevenzip,'
                 'username=szprobe,password=LocalProbeOnly427'])
    return target, lambda: command([*privilege, 'umount', str(target)])


def installed_device_probe(root, test, scenario, environment):
    """Keep the test-only QtTest DLL separate from the production payload."""
    binary = root.resolve() / 'bin'
    driver = binary / 'SevenzipMountDeviceTest.exe'
    if driver.exists():
        raise RuntimeError('A previous test driver exists; preserve it for inspection')
    cache = Path('build/dev/CMakeCache.txt').read_text().splitlines()
    qt = next(Path(line.split('=', 1)[1]).parents[2]
              for line in cache if line.startswith('Qt6_DIR:PATH='))
    test_libraries = list((qt / 'bin').glob('Qt6Test*.dll'))
    if not test_libraries:
        raise RuntimeError('The configured Qt kit has no test runtime')
    with tempfile.TemporaryDirectory(prefix='sevenzip-mount-test-runtime-') as temporary:
        for library in test_libraries:
            shutil.copy2(library, temporary)
        isolated = dict(environment)
        for name in ('QT_PLUGIN_PATH', 'QT_QPA_PLATFORM_PLUGIN_PATH',
                     'QML_IMPORT_PATH', 'QML2_IMPORT_PATH'):
            isolated.pop(name, None)
        system = Path(os.environ['SystemRoot'])
        isolated.update(PATH=os.pathsep.join(map(str, (binary, Path(temporary),
                                                       system / 'System32', system))),
                        QT_QPA_PLATFORM='windows',
                        SEVENZIP_SETTINGS_PATH=str(Path(temporary) / 'settings.ini'),
                        SEVENZIP_CODEC_MODULE_PATH=str(binary / '7z.dll'),
                        SEVENZIP_TEST_MOUNT_SERVER=str(binary / 'SevenzipSmb.exe'))
        shutil.copy2(test, driver)
        try:
            subprocess.run([str(driver), scenario, '-silent'], env=isolated, check=True)
        finally:
            driver.unlink()
        subprocess.run([str(binary / '7zFM.exe'), '--smoke-test'],
                       env=isolated, check=True, timeout=30)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server', type=Path)
    parser.add_argument('--product-client', action='store_true',
                        help='Exercise the production native-client broker and private IPC')
    parser.add_argument('--product-device', action='store_true',
                        help='Exercise native mounting, source loss and retry of a synthetic device')
    parser.add_argument('--installed-root', type=Path,
                        help='Consume an existing Windows installation with the device probe')
    parser.add_argument('--kernel-without-helper', action='store_true',
                        help='Linux protocol diagnostic only, using util-linux mount -i')
    parser.add_argument('--windows-global', action='store_true',
                        help='Use a temporary global mapping restricted to the current account')
    parser.add_argument('--trace', action='store_true', help='Log synthetic server diagnostics')
    args = parser.parse_args()
    if args.installed_root and (not args.product_device or sys.platform != 'win32'):
        parser.error('--installed-root requires --product-device on Windows')
    if args.product_client or args.product_device:
        if args.product_client and args.product_device:
            parser.error('Choose one product probe per invocation')
        if args.product_device and sys.platform not in ('darwin', 'win32'):
            parser.error('The native device probe currently supports macOS and Windows')
        target = 'archive_mount_device_tests' if args.product_device else 'native_mount_client_tests'
        scenario = 'nativeDeviceRemovalAndRetry' if args.product_device else 'nativeLifecycle'
        subprocess.run(['bash', 'scripts/build.sh', 'dev', target], check=True)
        test = Path('build/dev/' + target + ('.exe' if os.name == 'nt' else ''))
        environment = dict(os.environ, SEVENZIP_NATIVE_SMB_TEST='1')
        if args.installed_root:
            installed_device_probe(args.installed_root, test, scenario, environment)
        else:
            subprocess.run([str(test.resolve()), scenario, '-silent'],
                           env=environment, check=True)
        return
    if args.server is None:
        parser.error('--server is required unless a product probe is selected')
    # The runner has just synchronized source. Reuse its normal incremental
    # target before accepting native results from the executable.
    subprocess.run(['bash', 'scripts/build.sh', 'dev', 'smb2_mount_probe'], check=True)
    invocation = [str(args.server.resolve()), '0'] + (['--trace'] if args.trace else [])
    server = subprocess.Popen(invocation, stderr=subprocess.PIPE,
                              stdout=subprocess.DEVNULL, text=True)
    lines = queue.Queue()

    def receive():
        for line in server.stderr:
            lines.put(line)
        lines.put('server exited')

    receiver = threading.Thread(target=receive, daemon=True)
    receiver.start()
    temporary = Path(tempfile.mkdtemp(prefix='smb-native-', dir=Path.cwd() / 'build'))
    disconnect = None
    try:
        line = lines.get(timeout=15)
        ready = re.fullmatch(r'loopback SMB fixture ready on port ([0-9]+)\s*', line)
        if not ready:
            raise RuntimeError('Synthetic server not ready: ' + line)
        root, disconnect = mount(int(ready[1]), temporary.resolve(), args.kernel_without_helper,
                                 args.windows_global)
        check_contents(root)
    finally:
        if disconnect:
            try:
                disconnect()
            except Exception:
                print(f'Unmount failed; preserving synthetic server PID {server.pid} '
                      f'and fixture {temporary}', file=sys.stderr)
                raise
        elif os.path.ismount(temporary / 'volume'):
            raise RuntimeError(f'Unexpected live mount; preserving server PID {server.pid} '
                               f'and fixture {temporary}')
        server.terminate()
        try:
            server.wait(timeout=5)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait(timeout=5)
        receiver.join(timeout=1)
        diagnostics = deque(maxlen=100)
        while not lines.empty():
            diagnostics.append(lines.get_nowait().strip())
        for line in diagnostics:
            if line != 'server exited':
                print('Server: ' + line, flush=True)
        # Never recursively remove a possibly mounted tree, even after a failed
        # diagnostic. Keep failure evidence and let an explicit unmount retry.
        credentials = temporary / 'credentials'
        if credentials.exists():
            credentials.unlink()
        target = temporary / 'volume'
        if target.exists() and not os.path.ismount(target):
            target.rmdir()
        temporary.rmdir()


if __name__ == '__main__':
    try:
        main()
    except Exception as error:
        print('FAIL: ' + str(error), file=sys.stderr)
        sys.exit(1)
