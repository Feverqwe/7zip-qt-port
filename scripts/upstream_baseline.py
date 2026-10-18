#!/usr/bin/env python3
"""Verify the recorded 7-Zip source baseline and prepare an update review."""

import argparse
import json
import pathlib
import re
import subprocess
import sys


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
    parser.add_argument("command", choices=("inventory", "verify", "review"))
    parser.add_argument("--upstream", type=pathlib.Path)
    parser.add_argument("--manifest", type=pathlib.Path, default=MANIFEST)
    parser.add_argument("--new-revision", help="Proposed upstream commit for review")
    parser.add_argument("--path", action="append", default=[], help="Recorded local path to review")
    parser.add_argument("--output", type=pathlib.Path, help="Write review outside the source tree")
    args = parser.parse_args()
    try:
        data = load_manifest(args.manifest)
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
