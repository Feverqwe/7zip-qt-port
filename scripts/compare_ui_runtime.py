#!/usr/bin/env python3
"""Compare explicit native 7zFM/Qt capture pairs; emit agent-readable JSON only."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys

from check_ui_parity import read, relation, select


PLATFORMS = {"windows": ("winnt", {"windows"}),
             "linux": ("linux", {"xcb", "wayland", "wayland-egl"}),
             "macos": ("darwin", {"cocoa"})}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def vector(value, length):
    require(isinstance(value, list) and len(value) == length and
            all(type(x) in (int, float) and math.isfinite(x) for x in value),
            f"Expected {length} finite coordinates")
    require(all(x > 0 for x in value[-2:]), "Non-positive capture size")
    return value


def artifact(path):
    return {"path": str(path.resolve()),
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def compare_pair(base, case, target, contract, expected_exe):
    native_path, qt_path = base / case["reference"], base / target["report"]
    native, qt = read(native_path), read(qt_path)
    require(native.get("schemaVersion") == 1 and
            native.get("captureKind") == "application-runtime",
            "Reference must be an application-runtime capture, not a resource template")
    require(native["executable"]["sha256"] == expected_exe,
            "Reference executable hash differs from the reviewed baseline")
    require(native["language"] == case["language"] and native["state"] == case["state"],
            "Reference language/state differs from case")
    capture = qt["capture"]
    require(native["window"]["visible"] is True and qt["visible"] is True,
            "Root windows must be visible")
    platform = target["platform"]
    kernel, qpas = PLATFORMS[platform]
    require(capture.get("schema") == 1 and capture["platform"] == kernel and
            capture["qpa"] in qpas, "Qt capture is not native for the requested platform")
    require(capture["language"] == case["language"] and capture["state"] == case["state"],
            "Qt language/state differs from case")
    require(capture["coordinateSpace"] == "root-client-logical-pixels",
            "Unsupported Qt coordinate space")
    require(native["coordinateSpace"] ==
            "physical-screen-pixels; controls relative to root client origin",
            "Unsupported reference coordinate space")
    ns = vector(native["window"]["clientSizePx"], 2)
    qs = vector(qt["bounds"], 4)[2:]
    errors, measurements, adaptations, nodes = [], [], [], {}
    matched_qt = set()

    def issue(code, control, **details):
        errors.append({"code": code, "control": control, **details})

    for identity, entry in contract["controls"].items():
        if "omit" in entry:
            require(bool(entry["omit"]), "Omission requires reason")
            adaptations.append({"control": identity, "omit": entry["omit"]})
            continue
        require(type(entry.get("runtimeControlId")) is int,
                f"No reviewed runtimeControlId for {identity}")
        matches = [n for n in native["controls"]
                   if n["controlId"] == entry["runtimeControlId"] and
                   n["parentHandle"] == native["window"]["handle"]]
        if len(matches) != 1:
            issue("reference-control-match", identity, count=len(matches))
            continue
        try:
            node = select(qt, entry["selector"])
        except ValueError as error:
            issue("qt-control-match", identity, message=str(error))
            continue
        if id(node) in matched_qt:
            issue("reused-qt-control", identity)
            continue
        matched_qt.add(id(node))
        original = matches[0]
        nr, qr = vector(original["boundsRootClientPx"], 4), vector(node["bounds"], 4)
        nodes[identity] = (original, node)
        for prop in ("visible", "enabled"):
            require(type(original[prop]) is bool and type(node[prop]) is bool,
                    f"Missing boolean {prop}")
            if original[prop] != node[prop]:
                issue("state-difference", identity, property=prop,
                      reference=original[prop], actual=node[prop])
        if node["visible"]:
            if node.get("fullyVisible") is not True:
                issue("clipped-control", identity)
            if node.get("labelFits") is False:
                issue("clipped-text", identity)
        for key in ("textAdaptation", "classAdaptation"):
            if entry.get(key):
                adaptations.append({"control": identity, key: entry[key]})
        if (original["visible"] and not entry.get("textAdaptation") and
                original.get("text") is not None and
                node.get("text") is not None and original["text"] != node["text"]):
            issue("text-difference", identity,
                  reference=original["text"], actual=node["text"])
        normalized_native = [v / ns[i % 2] for i, v in enumerate(nr)]
        normalized_qt = [v / qs[i % 2] for i, v in enumerate(qr)]
        delta = [b - a for a, b in zip(normalized_native, normalized_qt)]
        measurements.append({"control": identity, "referenceBoundsPx": nr,
                             "qtBoundsLogicalPx": qr, "normalizedDelta": delta})
        if original["visible"] and node["visible"] and "maxNormalizedDelta" in case:
            if max(abs(v) for v in delta) > case["maxNormalizedDelta"]:
                issue("geometry-tolerance", identity, delta=delta,
                      tolerance=case["maxNormalizedDelta"])
    for rule in contract.get("relations", []):
        a, b, kind = rule["a"], rule["b"], rule["kind"]
        if rule.get("adaptation"):
            adaptations.append({"relation": rule, "reason": rule["adaptation"]})
        if a not in nodes or b not in nodes:
            continue
        for side, key, index in (("reference", "boundsRootClientPx", 0),
                                 ("qt", "bounds", 1)):
            left, right = nodes[a][index], nodes[b][index]
            if left["visible"] and right["visible"] and not relation(left[key], right[key], kind):
                issue("relation-difference", a, other=b, kind=kind, side=side)
    evidence = {"reference": artifact(native_path), "qt": artifact(qt_path)}
    for name, report_path, screenshot in (
            ("referenceScreenshot", native_path, native.get("screenshot")),
            ("qtScreenshot", qt_path, target.get("screenshot"))):
        if screenshot:
            evidence[name] = artifact(report_path.parent / screenshot)
    return {"case": case["id"], "platform": platform, "passed": not errors,
            "errors": errors, "measurements": measurements, "adaptations": adaptations,
            "clientSizes": {"referencePhysicalPx": ns, "qtLogicalPx": qs},
            "capture": {"referenceDpi": native["window"]["dpi"], "qt": capture},
            "evidence": evidence}


def compare_matrix(manifest_path):
    manifest_path = Path(manifest_path).resolve()
    base, manifest = manifest_path.parent, read(manifest_path)
    require(manifest.get("schema") == 1, "Unsupported matrix schema")
    contract_path = base / manifest["contract"]
    contract = read(contract_path)
    require(contract.get("schema") == 1, "Unsupported contract schema")
    require(bool(contract["controls"]), "Empty control mapping")
    ids = [entry.get("runtimeControlId") for entry in contract["controls"].values()
           if "omit" not in entry]
    require(bool(ids) and all(type(i) is int for i in ids) and len(set(ids)) == len(ids),
            "Runtime mapping requires unique integer IDs and at least one mapped control")
    required = manifest["requiredPlatforms"]
    require(bool(required) and len(set(required)) == len(required) and
            set(required) <= set(PLATFORMS), "Invalid requiredPlatforms")
    require(bool(manifest["cases"]), "Empty case matrix")
    require(len({c["id"] for c in manifest["cases"]}) == len(manifest["cases"]),
            "Duplicate case IDs")
    require(isinstance(manifest["referenceExeSha256"], str) and
            len(manifest["referenceExeSha256"]) == 64 and
            all(c in "0123456789abcdef" for c in manifest["referenceExeSha256"]),
            "Expected lowercase SHA-256 executable baseline")
    results, missing = [], []
    for case in manifest["cases"]:
        require(all(isinstance(case[k], str) and case[k] for k in
                    ("id", "language", "state", "reference")), "Invalid case identity")
        if "maxNormalizedDelta" in case:
            tolerance = case["maxNormalizedDelta"]
            require(type(tolerance) in (int, float) and math.isfinite(tolerance) and
                    0 <= tolerance <= 1, "Invalid geometry tolerance")
        targets = case["targets"]
        require(len({t["platform"] for t in targets}) == len(targets) and
                all(t["platform"] in PLATFORMS for t in targets), "Invalid/duplicate targets")
        for platform in required:
            if platform not in {t["platform"] for t in targets}:
                missing.append({"case": case["id"], "platform": platform})
        for target in targets:
            try:
                results.append(compare_pair(base, case, target, contract,
                                            manifest["referenceExeSha256"]))
            except (OSError, ValueError, KeyError, TypeError) as error:
                results.append({"case": case["id"], "platform": target["platform"],
                                "passed": False, "errors": [
                                    {"code": "invalid-capture", "message": str(error)}]})
    passed = not missing and all(r["passed"] for r in results)
    return {"schema": 1, "passed": passed, "missing": missing, "results": results,
            "evidence": {"manifest": artifact(manifest_path), "contract": artifact(contract_path)},
            "limitations": ["Only explicitly declared cases and mapped controls are checked",
                            "Language/state are declared by the capture producers",
                            "Normalized deltas are measurements unless a tolerance is supplied",
                            "No pixel, font, icon, focus, tab order or behavioral equivalence claim"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--matrix")
    mode.add_argument("--init-copy-matrix", metavar="PATH",
                      help="Create an eight-case EN/RU Copy/Move initial/compact manifest")
    parser.add_argument("--reference-exe-sha256", help="Reviewed original EXE hash for initialization")
    parser.add_argument("--output")
    args = parser.parse_args()
    try:
        if args.init_copy_matrix:
            require(bool(args.reference_exe_sha256), "Initialization requires --reference-exe-sha256")
            destination = Path(args.init_copy_matrix).resolve()
            contract = Path(__file__).resolve().parents[1] / "docs/ui-reference/copy-dialog.contract.json"
            matrix = {"schema": 1, "contract": str(contract),
                      "referenceExeSha256": args.reference_exe_sha256,
                      "requiredPlatforms": list(PLATFORMS), "cases": []}
            for move in (0, 1):
                for ru in (0, 1):
                    for size in ("initial", "compact"):
                        language, state = ("ru" if ru else "en"), f"{'move' if move else 'copy'}-{size}"
                        identity = f"{state}-{language}"
                        filename = f"copy-source-frame-{move}-{ru}-9-{size}.json"
                        matrix["cases"].append({
                            "id": identity, "language": language, "state": state,
                            "reference": f"reference/{identity}.json",
                            "targets": [{"platform": platform, "report": f"{platform}/{filename}"}
                                        for platform in PLATFORMS]})
            with destination.open("x", encoding="utf-8") as output:
                json.dump(matrix, output, ensure_ascii=False, indent=2)
                output.write("\n")
            sys.stdout.write(json.dumps({"schema": 1, "matrix": str(destination),
                                        "status": "capture-required", "cases": 8}) + "\n")
            return 0
        result = compare_matrix(args.matrix)
        code = 0 if result["passed"] else 1
    except (OSError, ValueError, KeyError, TypeError) as error:
        result, code = {"schema": 1, "passed": False, "errors": [
            {"code": "invalid-matrix", "message": str(error)}]}, 2
    data = json.dumps(result, ensure_ascii=False, indent=2, allow_nan=False) + "\n"
    if args.output:
        Path(args.output).write_text(data, encoding="utf-8")
    sys.stdout.write(data)
    return code


if __name__ == "__main__":
    sys.exit(main())
