"""Read and expand repository-owned dependency archives without fetching sources."""

import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import tarfile


def checked_path(root, name):
    relative = PurePosixPath(name)
    if (not relative.parts or relative.is_absolute() or ".." in relative.parts or
            str(relative) != name or "\\" in name or ":" in name):
        raise ValueError("Unsafe dependency path: " + name)
    result = root / relative
    if any(p.is_symlink() for p in (result, *result.parents)
           if p == root or root in p.parents):
        raise ValueError("Refusing linked dependency path: " + name)
    return result


class SourceArchives:
    def __init__(self, root, baseline=None):
        self.root = Path(root).resolve()
        if baseline is None:
            manifest = self.root / "docs/upstream-baseline.json"
            baseline = json.loads(manifest.read_text()) if manifest.is_file() else {}
        self.archives = baseline.get("dependency_archives", {})
        self._contents = {}
        self._owners = {}
        for name, entry in self.archives.items():
            if not re.fullmatch(r"[a-z0-9_-]+", name):
                raise ValueError("Invalid dependency archive name")
            checked_path(self.root, entry["local"])
            if not re.fullmatch(r"[0-9a-f]{64}", entry["sha256"]):
                raise ValueError("Invalid dependency archive hash")
            for member, digest in entry["files"].items():
                checked_path(self.root, member)
                if not re.fullmatch(r"[0-9a-f]{64}", digest):
                    raise ValueError("Invalid dependency member hash")
                logical = "third_party/" + name + "/" + member
                if logical in self._owners:
                    raise ValueError("Duplicate dependency source owner")
                self._owners[logical] = (name, member)
            for member, owner in entry.get("overlays", {}).items():
                checked_path(self.root, member)
                checked_path(self.root, owner)
                if member in entry["files"] or not owner.startswith("src/"):
                    raise ValueError("Adaptations require a distinct project source owner")
            for member, owner in entry.get("asset_overlays", {}).items():
                checked_path(self.root, member)
                checked_path(self.root, owner)
                if member not in entry["files"] or not owner.startswith("src/"):
                    raise ValueError("Asset adaptations require a recorded original and project owner")

    def _read_archive(self, name):
        if name in self._contents:
            return self._contents[name]
        entry = self.archives[name]
        source = checked_path(self.root, entry["local"])
        if hashlib.sha256(source.read_bytes()).hexdigest() != entry["sha256"]:
            raise ValueError("Dependency archive differs from baseline: " + name)
        contents = {}
        with tarfile.open(source, "r:*") as archive:
            for member in archive:
                if member.isdir():
                    checked_path(self.root, member.name)
                    continue
                if (not member.isfile() or member.name in contents or
                        member.name not in entry["files"]):
                    raise ValueError("Unsafe or unrecorded dependency member: " + member.name)
                checked_path(self.root, member.name)
                content = archive.extractfile(member).read()
                if hashlib.sha256(content).hexdigest() != entry["files"][member.name]:
                    raise ValueError("Dependency member differs from baseline: " + member.name)
                contents[member.name] = (content, 0o755 if member.mode & 0o111 else 0o644)
        if set(contents) != set(entry["files"]):
            raise ValueError("Dependency archive omits recorded inputs: " + name)
        self._contents[name] = contents
        return contents

    def contains(self, owner):
        return owner in self._owners or checked_path(self.root, owner).is_file()

    def is_archived(self, owner):
        return owner in self._owners

    def read_bytes(self, owner):
        # Packed owners are always read from the verified archive, even if an
        # obsolete expanded copy happens to exist in the checkout.
        if owner in self._owners:
            name, member = self._owners[owner]
            return self._read_archive(name)[member][0]
        return checked_path(self.root, owner).read_bytes()

    def prepare(self, destination):
        destination = Path(destination)
        if destination.is_symlink():
            raise ValueError("Refusing linked dependency directory")
        destination.mkdir(parents=True, exist_ok=True)
        marker = checked_path(destination, ".sevenzip-dependencies.json")
        # The common directory also holds the independently owned 7-Zip view.
        previous = (json.loads(marker.read_text()) if marker.exists() else
                    {"producer": "sevenzip-dependencies", "files": []})
        if previous.get("producer", "sevenzip-dependencies") != "sevenzip-dependencies":
            raise ValueError("Unexpected dependency directory owner")
        prepared = {}
        for name, entry in self.archives.items():
            subtree = checked_path(destination, name)
            if not marker.exists() and subtree.exists() and any(subtree.iterdir()):
                raise ValueError("Refusing unowned dependency directory: " + name)
            prepared[name] = dict(self._read_archive(name))
            for member, owner in entry.get("overlays", {}).items():
                source = checked_path(self.root, owner)
                if not source.is_file():
                    raise ValueError("Missing adapted dependency: " + owner)
                prepared[name][member] = (('#include "' + source.as_posix() + '"\n').encode(), 0o644)
            for member, owner in entry.get("asset_overlays", {}).items():
                prepared[name][member] = (checked_path(self.root, owner).read_bytes(), 0o644)
        if not marker.exists():
            marker.write_text(json.dumps(previous) + "\n")
        files = []
        for name, contents in prepared.items():
            for member, (content, mode) in contents.items():
                logical = name + "/" + member
                output = checked_path(destination, logical)
                output.parent.mkdir(parents=True, exist_ok=True)
                if not output.exists() or output.read_bytes() != content:
                    output.write_bytes(content)
                    output.chmod(mode)
                files.append(logical)
        for obsolete in set(previous["files"]) - set(files):
            checked_path(destination, obsolete).unlink(missing_ok=True)
        marker.write_text(json.dumps({"producer": "sevenzip-dependencies", "files": sorted(files)}) + "\n")
