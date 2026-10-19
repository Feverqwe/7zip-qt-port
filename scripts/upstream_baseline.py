#!/usr/bin/env python3
"""Verify the recorded 7-Zip source baseline and prepare an update review."""

import argparse
import hashlib
import json
import pathlib
import posixpath
import re
import subprocess
import sys
import tarfile


ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs/upstream-baseline.json"


def git(repo, *args):
    result = subprocess.run(["git", "-C", str(repo), *args], capture_output=True)
    if result.returncode:
        raise ValueError(result.stderr.decode("utf-8", "replace").strip())
    return result.stdout


def source_path(path):
    if path == "third_party/7zip/License.txt":
        return "DOC/License.txt"
    if path.startswith("third_party/7zip/"):
        return path[len("third_party/7zip/") :]
    if path.startswith("src/CPP/") or path.startswith("src/C/"):
        return path[len("src/") :]
    raise ValueError("Unexpected borrowed path: " + path)


def load_manifest(path):
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("schema") != 1 or not re.fullmatch(r"[0-9a-f]{40}", data.get("revision", "")):
        raise ValueError("Manifest requires schema 1 and a full commit hash")
    if data.get("repository") != "https://github.com/ip7z/7zip.git":
        raise ValueError("Unexpected upstream repository")
    groups = [data.get(name) for name in ("exact", "adapted", "local")]
    if any(not isinstance(group, list) or group != sorted(set(group)) for group in groups):
        raise ValueError("Manifest path groups must be sorted, unique lists")
    paths = [path for group in groups for path in group]
    if len(paths) != len(set(paths)):
        raise ValueError("Borrowed paths have multiple owners")
    sources = [source_path(path) for path in paths]
    if len(sources) != len(set(sources)):
        raise ValueError("Upstream module has multiple authoritative implementations")
    overrides = data.get("overrides", {})
    if not isinstance(overrides, dict) or set(overrides) - set(paths) or any(
        not re.fullmatch(r"[0-9a-f]{40}", value) for value in overrides.values()
    ):
        raise ValueError("Revision overrides require recorded paths and full hashes")
    return data


def tracked_paths():
    paths = git(ROOT, "ls-files", "--cached", "--others", "--exclude-standard",
                "third_party/7zip/", "src/CPP/", "src/C/").decode().splitlines()
    return {path for path in paths
            if path != "third_party/7zip/README.md" and (ROOT / path).is_file()}


def check_inventory(data):
    recorded = set(data["exact"] + data["adapted"] + data["local"])
    current = tracked_paths()
    missing = current - recorded
    stale = recorded - current
    if missing or stale:
        raise ValueError(f"Inventory mismatch: unrecorded={sorted(missing)}, stale={sorted(stale)}")


def runtime_header_dependencies(snapshot, members, overlays):
    """Supplement upstream make/nmake rules for adapted header consumers."""
    sources = {name for name in members if pathlib.PurePosixPath(name).suffix in
               (".c", ".cpp", ".h", ".hpp", ".inl")}
    headers = {name for name in overlays if pathlib.PurePosixPath(name).suffix in
               (".h", ".hpp", ".inl")}
    reverse = {}
    pattern = re.compile(r'^\s*#\s*include\s*["<]([^">]+)[">]', re.MULTILINE)
    for name in sources:
        for include in pattern.findall((snapshot / name).read_text(errors="replace")):
            local = posixpath.normpath(posixpath.join(posixpath.dirname(name), include))
            # Relative includes have one owner. For include-directory searches,
            # retain all possible source-tree matches rather than miss a consumer.
            matches = [local] if local in sources else [path for path in sources
                                                        if path == include or path.endswith("/" + include)]
            for dependency in matches:
                reverse.setdefault(dependency, set()).add(name)
    dependencies = {}
    for header in headers:
        pending = [header]
        seen = set()
        while pending:
            current = pending.pop()
            if current in seen:
                continue
            seen.add(current)
            if pathlib.PurePosixPath(current).suffix in (".c", ".cpp"):
                dependencies.setdefault(pathlib.PurePosixPath(current).stem, set()).add(header)
            pending.extend(reverse.get(current, ()))
    for directory in ("CPP/7zip/UI/Console", "CPP/7zip/Bundles/Format7zF", "CPP/7zip/Bundles/Alone2"):
        for windows, original, generated in ((False, "makefile.gcc", "sevenzip-runtime.gcc.mak"),
                                              (True, "makefile", "sevenzip-runtime.mak")):
            bundle = snapshot / directory
            if not (bundle / original).is_file():
                continue
            lines = [f'!include "{original}"' if windows else f'include {original}']
            for stem, needed in sorted(dependencies.items()):
                relative = sorted(posixpath.relpath(name, directory) for name in needed)
                target = "$(O)\\" + stem + ".obj" if windows else "$(O)/" + stem + ".o"
                lines.append(target + ": " + " ".join('"' + name + '"' if windows else name
                                                       for name in relative))
            content = "\n".join(lines) + "\n"
            output = bundle / generated
            if output.is_symlink():
                raise ValueError("Refusing linked runtime dependency makefile")
            if not output.exists() or output.read_text() != content:
                output.write_text(content)


def prepare_runtime_snapshot(data, snapshot, upstream):
    """Overlay only engine adaptations explicitly selected in the baseline."""
    paths = data.get("runtime_adapted", [])
    if not isinstance(paths, list) or paths != sorted(set(paths)) or set(paths) - set(data["adapted"]):
        raise ValueError("Runtime sources must be selected adapted upstream modules")
    snapshot.mkdir(parents=True, exist_ok=True)
    owner = snapshot / ".sevenzip-runtime-source.json"
    if snapshot.is_symlink() or (any(snapshot.iterdir()) and not owner.is_file()):
        raise ValueError("Refusing an unowned runtime source directory")
    previous = json.loads(owner.read_text()) if owner.exists() else {"files": []}
    if previous.get("producer", "sevenzip-runtime-source") != "sevenzip-runtime-source":
        raise ValueError("Unexpected runtime source owner")
    if not owner.exists():
        owner.write_text(json.dumps({"producer": "sevenzip-runtime-source", "files": []}) + "\n")
    overlays = {source_path(path): ROOT / path for path in paths}
    members = []
    process = subprocess.Popen(["git", "-C", str(upstream), "archive", data["revision"]],
                               stdout=subprocess.PIPE)
    try:
        with tarfile.open(fileobj=process.stdout, mode="r|") as archive:
            for member in archive:
                if member.isdir():
                    continue
                relative = pathlib.PurePosixPath(member.name)
                if not member.isfile() or relative.is_absolute() or ".." in relative.parts:
                    raise ValueError("Unsafe runtime source entry: " + member.name)
                destination = snapshot / relative
                source = overlays.get(member.name)
                content = source.read_bytes() if source else archive.extractfile(member).read()
                if destination.is_symlink() or (source and source.is_symlink()):
                    raise ValueError("Refusing linked runtime source: " + member.name)
                destination.parent.mkdir(parents=True, exist_ok=True)
                if not destination.exists() or destination.read_bytes() != content:
                    destination.write_bytes(content)
                    destination.chmod(member.mode)
                members.append(member.name)
        if process.wait():
            raise ValueError("Unable to retrieve pinned runtime source")
    finally:
        process.stdout.close()
        if process.poll() is None:
            process.terminate()
            process.wait()
    # Remove only previously owned source paths; retain all incremental outputs.
    for name in set(previous["files"]) - set(members):
        relative = pathlib.PurePosixPath(name)
        if relative.is_absolute() or ".." in relative.parts:
            raise ValueError("Unsafe previous runtime source entry")
        (snapshot / relative).unlink(missing_ok=True)
    owner.write_text(json.dumps({"producer": "sevenzip-runtime-source", "files": members}) + "\n")
    hashes = {}
    for path in paths:
        source = ROOT / path
        destination = snapshot / source_path(path)
        if source.is_symlink() or destination.is_symlink() or not destination.is_file():
            raise ValueError("Runtime adaptation requires regular source files: " + path)
        hashes[path] = hashlib.sha256(destination.read_bytes()).hexdigest()
    (snapshot / "runtime-adaptations.json").write_text(json.dumps(hashes, indent=2) + "\n")
    runtime_header_dependencies(snapshot, members, overlays)


def verify(data, upstream):
    check_inventory(data)
    revisions = {data["revision"], *data.get("overrides", {}).values()}
    for revision in revisions:
        if git(upstream, "rev-parse", "--verify", revision + "^{commit}").decode().strip() != revision:
            raise ValueError("Recorded revision is unavailable: " + revision)
    for path in data["exact"] + data["adapted"]:
        revision = data.get("overrides", {}).get(path, data["revision"])
        original = git(upstream, "show", revision + ":" + source_path(path))
        if path in data["exact"] and (ROOT / path).read_bytes() != original:
            raise ValueError("Exact vendor differs from baseline: " + path)
    for path in data["local"]:
        revision = data.get("overrides", {}).get(path, data["revision"])
        result = subprocess.run(
            ["git", "-C", str(upstream), "cat-file", "-e", revision + ":" + source_path(path)],
            capture_output=True,
        )
        if result.returncode == 0:
            raise ValueError("Local-only path now has an upstream counterpart: " + path)
    print(
        f"Verified {len(data['exact'])} exact, {len(data['adapted'])} adapted, "
        f"{len(data['local'])} local paths across {len(revisions)} revision(s)"
    )


def review(data, upstream, new_revision, paths):
    new = git(upstream, "rev-parse", "--verify", new_revision + "^{commit}").decode().strip()
    selected = paths or data["exact"] + data["adapted"]
    known = set(data["exact"] + data["adapted"])
    if set(selected) - known:
        raise ValueError("Review path absent from borrowed manifest")
    rows = [f"# Upstream update review: proposed {new}", ""]
    rows.append("Review changed dependencies and local adaptations before updating provenance.")
    rows.append("This report does not modify the port or sibling upstream checkout.\n")
    for path in selected:
        old = data.get("overrides", {}).get(path, data["revision"])
        source = source_path(path)
        status = git(upstream, "diff", "--name-status", old, new, "--", source).decode().strip()
        if not status:
            continue
        kind = "exact" if path in data["exact"] else "adapted"
        rows.append(f"- `{path}` ({kind}, `{old}` → `{new}`): `{status}`")
        if kind == "adapted":
            rows.append("  Review the upstream diff against the current Qt/platform adaptations.")
        else:
            rows.append("  Replace with exact new source only after dependency and build review.")
    if len(rows) == 4:
        rows.append("No selected upstream paths changed.")
    return "\n".join(rows) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("inventory", "verify", "review", "runtime-snapshot"))
    parser.add_argument("--snapshot", type=pathlib.Path)
    parser.add_argument("--upstream", type=pathlib.Path)
    parser.add_argument("--manifest", type=pathlib.Path, default=MANIFEST)
    parser.add_argument("--new-revision", help="Proposed upstream commit for review")
    parser.add_argument("--path", action="append", default=[], help="Recorded local path to review")
    parser.add_argument("--output", type=pathlib.Path, help="Write review outside the source tree")
    args = parser.parse_args()
    try:
        data = load_manifest(args.manifest)
        if args.command == "runtime-snapshot":
            if not args.snapshot or not args.upstream:
                raise ValueError("Runtime snapshot and upstream directories are required")
            prepare_runtime_snapshot(data, args.snapshot, args.upstream)
            return 0
        if args.command == "inventory":
            check_inventory(data)
            print("Recorded inventory matches current borrowed sources")
            return 0
        if not args.upstream:
            raise ValueError("Verify and review require --upstream")
        verify(data, args.upstream)
        if args.command == "review":
            if not args.new_revision or not args.output:
                raise ValueError("Review requires --new-revision and --output")
            report = review(data, args.upstream, args.new_revision, args.path)
            if args.output.exists():
                raise ValueError("Review output already exists: " + str(args.output))
            args.output.write_text(report, encoding="utf-8")
            print("Wrote " + str(args.output))
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        print("upstream_baseline: " + str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
