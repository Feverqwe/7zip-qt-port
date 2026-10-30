"""Copy an isolated official console deployment with its compiler runtimes."""

import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import time


def copy_codec_runtime(source, destination):
    source, destination = Path(source), Path(destination)
    names = ("7z.exe", "7z.dll") if sys.platform == "win32" else ("7z", "7z.so")
    for name in names:
        shutil.copy2(source / name, destination / name)
    # Windows 7-Zip deliberately excludes PATH from dependent DLL searches.
    # CMake supplies the selected toolchain's CRT/ASan files for test fixtures.
    for library in filter(None, os.environ.get("SEVENZIP_TEST_RUNTIME_LIBRARIES", "").split("|")):
        path = Path(library)
        shutil.copy2(path, destination / path.name)


def run_codec_command(arguments, *, timeout, memory_limit=None):
    if memory_limit is None:
        return subprocess.run(arguments, stdin=subprocess.DEVNULL, capture_output=True,
                              timeout=timeout)
    if os.environ.get("SEVENZIP_TEST_SANITIZERS") != "1":
        def limit_address_space():
            import resource
            resource.setrlimit(resource.RLIMIT_AS, (memory_limit, memory_limit))
        return subprocess.run(arguments, stdin=subprocess.DEVNULL, capture_output=True,
                              timeout=timeout, preexec_fn=limit_address_space)
    # ASan reserves terabytes of virtual shadow address space. Bound actual
    # resident memory instead, without disabling instrumentation or its checks.
    if sys.platform != "linux":
        raise RuntimeError("Sanitizer resident-memory fixtures require Linux")
    timer = Path("/usr/bin/time")
    if not timer.is_file():
        raise RuntimeError("GNU time is required for sanitizer memory-bound fixtures")
    with tempfile.TemporaryDirectory(prefix="sevenzip-memory-") as temporary:
        metric = Path(temporary) / "peak-kib"
        command = [str(timer), "-f", "%M", "-o", str(metric), "--", *arguments]
        with subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, start_new_session=True) as process:
            deadline = time.monotonic() + timeout
            try:
                while True:
                    remaining = deadline - time.monotonic()
                    if remaining <= 0:
                        raise subprocess.TimeoutExpired(arguments, timeout)
                    try:
                        stdout, stderr = process.communicate(timeout=min(0.05, remaining))
                        break
                    except subprocess.TimeoutExpired:
                        children = Path(f"/proc/{process.pid}/task/{process.pid}/children")
                        try:
                            child_pids = children.read_text().split()
                        except FileNotFoundError:
                            child_pids = []
                        for child in child_pids:
                            try:
                                status = Path(f"/proc/{child}/status").read_text().splitlines()
                            except FileNotFoundError:
                                continue
                            peak = next((int(line.split()[1]) * 1024 for line in status
                                         if line.startswith("VmHWM:")), 0)
                            if peak > memory_limit:
                                raise MemoryError(f"Console resident memory exceeded {memory_limit} bytes")
                # GNU time also catches short-lived peaks between samples.
                peak = int(metric.read_text().splitlines()[-1]) * 1024
                if peak > memory_limit:
                    raise MemoryError(f"Console peak resident memory {peak} exceeded {memory_limit} bytes")
                return subprocess.CompletedProcess(arguments, process.returncode, stdout, stderr)
            except BaseException:
                if process.poll() is None:
                    try:
                        os.killpg(process.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                process.communicate()
                raise
