#!/usr/bin/env python3
"""Extract an upstream reference or compare a reviewed contract with Qt reports."""
import argparse
import json
import subprocess
import sys
from pathlib import Path

from ui_reference import extract


def read(path):
    return json.loads(Path(path).read_text(encoding="utf-8"))


def flatten(node):
    yield node
    for child in node.get("children", []):
        yield from flatten(child)


def select(root, selector):
    if not selector or set(selector) - {"name", "class", "text", "within"}:
        raise ValueError("Invalid widget selector: " + repr(selector))
    roots = [root]
    if "within" in selector:
        roots = [n for n in flatten(root) if n.get("name") == selector["within"]]
    matches = []
    for scope in roots:
        for node in flatten(scope):
            if all(node.get(k) in (v if isinstance(v, list) else [v])
                   for k, v in selector.items() if k != "within"):
                matches.append(node)
    if len(matches) != 1:
        raise ValueError(f"Selector {selector} matched {len(matches)} widgets (expected one)")
    return matches[0]


def relation(a, b, kind, reference=False):
    x, y, w, h = a
    bx, by, bw, bh = b
    if kind == "above":
        return y < by if reference else y + h <= by
    if kind == "left_of":
        return x < bx if reference else x + w <= bx
    if kind == "inside":
        return x >= bx and y >= by and x + w <= bx + bw and y + h <= by + bh
    if kind == "separate":
        if reference:
            raise ValueError("separate is a Qt usability assertion; use above/left_of for RC")
        return x + w <= bx or bx + bw <= x or y + h <= by or by + bh <= y
    raise ValueError("Unknown relation: " + kind)


def compare(reference, contract, reports):
    errors, deviations, checked = [], [], []
    if reference.get("schema") != 1 or contract.get("schema") != 1:
        raise ValueError("Unsupported schema")
    if reference["upstreamCommit"] != contract["upstreamCommit"]:
        raise ValueError("Contract upstream commit differs from reference")
    if reference["dependencies"] != contract["dependencies"]:
        raise ValueError("Reference inputs changed; review contract and regenerate baseline")
    dialogs = [d for d in reference["dialogs"] if d["id"] == contract["dialog"]]
    if len(dialogs) != 1:
        raise ValueError("Contract dialog not found uniquely in reference")
    controls = {c["id"]: c for c in dialogs[0]["controls"]}
    if len(controls) != len(dialogs[0]["controls"]):
        raise ValueError("Duplicate resource IDs; this dialog needs a more specific contract")
    mapping = contract["controls"]
    for identity in set(controls) - set(mapping):
        errors.append("Unreviewed upstream control: " + identity)
    for identity in set(mapping) - set(controls):
        errors.append("Unknown upstream control in mapping: " + identity)
    for identity, entry in mapping.items():
        if "omit" in entry:
            if not entry["omit"] or "selector" in entry:
                raise ValueError("Omission requires a reason and no selector: " + identity)
            deviations.append(identity + ": omitted: " + entry["omit"])
        for key in ("classAdaptation", "textAdaptation"):
            if key in entry:
                if not entry[key]:
                    raise ValueError("Adaptation needs a reason: " + identity)
                deviations.append(identity + ": " + entry[key])
    # Every state must cite reviewed upstream evidence. Static reports do not
    # prove transitions or emitted results; those stay in behavioral Qt tests.
    for state in contract["states"]:
        if not state.get("evidence"):
            raise ValueError("State has no upstream evidence: " + state["name"])
        for evidence in state["evidence"]:
            if evidence["source"] not in reference["dependencies"] or not evidence.get("symbol"):
                raise ValueError("Evidence must name an extracted dependency and symbol")
            if evidence["symbol"] not in reference.get("behaviorSymbols", {}).get(evidence["source"], []):
                raise ValueError("Evidence symbol absent from source index: " + evidence["symbol"])
        for filename in state["reports"]:
            path = Path(reports) / filename
            if not path.is_file():
                errors.append("Missing state report: " + str(path))
                continue
            root, widgets = read(path), {}
            for identity, entry in mapping.items():
                if "omit" in entry or identity not in controls:
                    continue
                try:
                    node = select(root, entry["selector"])
                    widgets[identity] = node
                    expected = entry.get("qtClass", controls[identity]["qtClass"])
                    if entry.get("qtClass") and not entry.get("classAdaptation"):
                        raise ValueError("qtClass override requires classAdaptation")
                    if expected is None:
                        raise ValueError("Unresolved CONTROL class requires reviewed qtClass")
                    if node["class"] != expected:
                        errors.append(f"{filename}: {identity}: class {node['class']} != {expected}")
                    if "editable" in controls[identity] and node.get("editable") != controls[identity]["editable"]:
                        errors.append(f"{filename}: {identity}: combo editability differs")
                    text = controls[identity]["text"]
                    if text and not entry.get("textAdaptation") and node.get("text") != text:
                        errors.append(f"{filename}: {identity}: text differs from upstream")
                except (ValueError, KeyError) as error:
                    errors.append(f"{filename}: {identity}: {error}")
            for assertion in state.get("assert", []):
                identity, prop = assertion["id"], assertion["property"]
                if prop not in {"visible", "enabled", "fullyVisible", "editable", "labelFits", "itemCount", "text"}:
                    raise ValueError("Unsupported report property: " + prop)
                if identity not in widgets or widgets[identity].get(prop) != assertion["equals"]:
                    errors.append(f"{filename}: {identity}: expected {prop}={assertion['equals']!r}")
            for rule in contract.get("relations", []):
                a, b, kind = rule["a"], rule["b"], rule["kind"]
                if rule.get("adaptation"):
                    deviations.append(f"{a} {kind} {b}: " + rule["adaptation"])
                elif not relation(controls[a]["boundsDLU"], controls[b]["boundsDLU"], kind, True):
                    raise ValueError(f"Relation not supported by RC: {a} {kind} {b}")
                if a not in widgets or b not in widgets or not relation(
                        widgets[a]["bounds"], widgets[b]["bounds"], kind):
                    errors.append(f"{filename}: layout differs: {a} {kind} {b}")
            checked.append({"state": state["name"], "report": filename})
    if not checked:
        errors.append("No state reports checked")
    return {"schema": 1, "passed": not errors, "checked": checked, "errors": errors,
            "deviations": sorted(set(deviations)),
            "limitations": ["Reviewed subset of geometry and state properties only",
                            "Transitions/results require listed behavioral tests",
                            "No pixel parity or Windows runtime verification"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    export = commands.add_parser("extract")
    export.add_argument("--upstream", required=True)
    export.add_argument("--resource", required=True)
    export.add_argument("--source", action="append", default=[])
    export.add_argument("--compiler", help="clang/gcc executable path")
    export.add_argument("--output", required=True)
    check = commands.add_parser("compare")
    check.add_argument("--reference", required=True)
    check.add_argument("--contract", required=True)
    check.add_argument("--reports", required=True)
    check.add_argument("--output")
    args = parser.parse_args()
    try:
        if args.command == "extract":
            result = extract(args.upstream, args.resource, args.source, args.compiler)
            Path(args.output).write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n")
            print(f"Extracted {len(result['dialogs'])} dialog(s) to {args.output}")
            return 0
        result = compare(read(args.reference), read(args.contract), args.reports)
        if args.output:
            Path(args.output).write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n")
        for error in result["errors"]:
            print(error, file=sys.stderr)
        print(f"{'PASS' if result['passed'] else 'FAIL'}: {len(result['checked'])} reports; "
              f"{len(result['deviations'])} recorded adaptations")
        return 0 if result["passed"] else 1
    except (ValueError, KeyError, OSError, SyntaxError, subprocess.CalledProcessError) as error:
        print(str(error), file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
