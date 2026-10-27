"""Optional local timestamp memo: loss causes retransfers, never missing source."""

from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import shutil


def prepare(source, destination, cache, guest_time_ns):
    try:
        previous = json.loads(cache.read_text())
        if not isinstance(previous, dict):
            previous = {}
    except (OSError, ValueError):
        previous = {}
    pending = []

    def copy_version(source_path, target):
        shutil.copy2(source_path, target)
        path = Path(target)
        relative = path.relative_to(destination).as_posix()
        info = path.stat()
        identity = [info.st_size, info.st_mtime_ns]
        if relative in (".runner-context.json", ".runner-source.json"):
            # These two generated descriptors are recreated for each snapshot;
            # compare their small contents, not their fresh creation times.
            identity[1] = hashlib.sha256(path.read_bytes()).hexdigest()
        entry = previous.get(relative)
        # The cache describes source versions, never receiver existence. cp2
        # still examines the actual destination on every invocation. New
        # versions get guest time AFTER its preceding build/branch lock.
        if (isinstance(entry, list) and len(entry) == 3 and entry[:2] == identity
                and isinstance(entry[2], int) and 0 < entry[2] <= guest_time_ns):
            stamp = entry[2]
        else:
            stamp = guest_time_ns // 100 * 100  # NTFS precision; stable on Unix too.
        os.utime(path, ns=(info.st_atime_ns, stamp))
        return relative, [*identity, stamp]

    with ThreadPoolExecutor(max_workers=8) as pool:
        def enqueue(source_path, target):
            pending.append(pool.submit(copy_version, source_path, target))
            return target

        try:
            shutil.copytree(source, destination, symlinks=True, copy_function=enqueue)
            current = dict(future.result() for future in pending)
        except BaseException:
            pool.shutdown(wait=True, cancel_futures=True)
            raise
    return current


def commit(cache, entries):
    cache.parent.mkdir(parents=True, exist_ok=True)
    temporary = cache.with_suffix(".tmp")
    temporary.write_text(json.dumps(entries) + "\n")
    temporary.replace(cache)
