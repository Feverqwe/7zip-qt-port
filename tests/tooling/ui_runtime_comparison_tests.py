"""Public CLI regressions for native geometry evidence and incomplete matrices."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


TOOL = Path(__file__).resolve().parents[2] / "scripts/compare_ui_runtime.py"


class RuntimeComparisonTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.contract = {
            "schema": 1, "controls": {
                "label": {"runtimeControlId": 100, "selector": {"name": "label"}},
                "button": {"runtimeControlId": 1, "selector": {"name": "button"}}},
            "relations": [{"a": "label", "b": "button", "kind": "above"}]}
        self.native = {
            "schemaVersion": 1, "captureKind": "application-runtime",
            "coordinateSpace": "physical-screen-pixels; controls relative to root client origin",
            "executable": {"sha256": "a" * 64}, "language": "ru", "state": "copy-normal",
            "window": {"handle": "root", "visible": True, "clientSizePx": [400, 200], "dpi": 192},
            "controls": [
                {"controlId": 100, "parentHandle": "root", "visible": True,
                 "enabled": True, "text": "Копировать", "boundsRootClientPx": [20, 20, 160, 40]},
                {"controlId": 1, "parentHandle": "root", "visible": True,
                 "enabled": True, "text": "OK", "boundsRootClientPx": [20, 120, 80, 40]}]}
        self.qt = {
            "bounds": [0, 0, 200, 100], "visible": True,
            "capture": {"schema": 1, "platform": "darwin", "qpa": "cocoa",
                        "language": "ru", "state": "copy-normal",
                        "coordinateSpace": "root-client-logical-pixels"},
            "children": [
                {"name": "label", "bounds": [10, 10, 80, 20], "visible": True,
                 "enabled": True, "fullyVisible": True, "labelFits": True, "text": "Копировать"},
                {"name": "button", "bounds": [10, 60, 40, 20], "visible": True,
                 "enabled": True, "fullyVisible": True, "text": "OK"}]}
        self.matrix = {
            "schema": 1, "contract": "contract.json", "referenceExeSha256": "a" * 64,
            "requiredPlatforms": ["macos"], "cases": [
                {"id": "copy-ru-normal", "language": "ru", "state": "copy-normal",
                 "reference": "native.json", "maxNormalizedDelta": 0.01,
                 "targets": [{"platform": "macos", "report": "qt.json"}]}]}

    def run_tool(self):
        for name, data in (("contract", self.contract), ("native", self.native),
                           ("qt", self.qt), ("matrix", self.matrix)):
            (self.root / (name + ".json")).write_text(json.dumps(data), encoding="utf-8")
        result = subprocess.run([sys.executable, str(TOOL), "--matrix",
                                 str(self.root / "matrix.json")], capture_output=True, text=True)
        return result.returncode, json.loads(result.stdout)

    def codes(self, report):
        return {e["code"] for r in report["results"] for e in r["errors"]}

    def test_physical_and_logical_coordinates_with_different_scale(self):
        code, report = self.run_tool()
        self.assertEqual(code, 0)
        self.assertTrue(report["passed"])
        self.assertEqual(report["results"][0]["measurements"][0]["normalizedDelta"], [0] * 4)

    def test_missing_platform_is_not_acceptance(self):
        self.matrix["requiredPlatforms"] = ["windows", "linux", "macos"]
        code, report = self.run_tool()
        self.assertEqual(code, 1)
        self.assertEqual({x["platform"] for x in report["missing"]}, {"windows", "linux"})

    def test_clipping_state_and_relation_failures_are_actionable(self):
        self.qt["children"][0].update(labelFits=False, fullyVisible=False, enabled=False,
                                      bounds=[10, 70, 80, 20])
        code, report = self.run_tool()
        self.assertEqual(code, 1)
        self.assertTrue({"clipped-text", "clipped-control", "state-difference",
                         "relation-difference", "geometry-tolerance"} <= self.codes(report))

    def test_measurements_do_not_impose_unreviewed_geometry_threshold(self):
        del self.matrix["cases"][0]["maxNormalizedDelta"]
        self.qt["children"][0]["bounds"][2] = 90
        code, report = self.run_tool()
        self.assertEqual(code, 0)
        self.assertNotEqual(report["results"][0]["measurements"][0]["normalizedDelta"], [0] * 4)

    def test_ambiguous_and_missing_controls_fail(self):
        for change in ("qt-duplicate", "native-duplicate", "missing"):
            with self.subTest(change=change):
                qt, native = copy.deepcopy(self.qt), copy.deepcopy(self.native)
                if change == "qt-duplicate":
                    self.qt["children"].append(copy.deepcopy(self.qt["children"][0]))
                elif change == "native-duplicate":
                    self.native["controls"].append(copy.deepcopy(self.native["controls"][0]))
                else:
                    self.native["controls"] = []
                code, report = self.run_tool()
                self.assertEqual(code, 1)
                self.assertTrue(self.codes(report) & {"reference-control-match", "qt-control-match"})
                self.qt, self.native = qt, native

    def test_two_source_controls_cannot_reuse_one_qt_widget(self):
        self.contract["controls"]["button"]["selector"] = {"name": "label"}
        code, report = self.run_tool()
        self.assertEqual(code, 1)
        self.assertIn("reused-qt-control", self.codes(report))

    def test_text_difference_requires_explicit_adaptation(self):
        self.qt["children"][0]["text"] = "Move"
        code, report = self.run_tool()
        self.assertEqual(code, 1)
        self.assertIn("text-difference", self.codes(report))
        self.contract["controls"]["label"]["textAdaptation"] = "Reviewed platform wording"
        code, report = self.run_tool()
        self.assertEqual(code, 0)
        self.assertTrue(report["results"][0]["adaptations"])

    def test_resource_preview_offscreen_and_wrong_baseline_are_rejected(self):
        for change in ("template", "offscreen", "baseline", "language", "qt-state", "zero-size", "nan"):
            with self.subTest(change=change):
                qt, native = copy.deepcopy(self.qt), copy.deepcopy(self.native)
                if change == "template":
                    self.native["captureKind"] = "resource-template"
                elif change == "offscreen":
                    self.qt["capture"]["qpa"] = "offscreen"
                elif change == "baseline":
                    self.native["executable"]["sha256"] = "b" * 64
                elif change == "language":
                    self.native["language"] = "en"
                elif change == "qt-state":
                    self.qt["capture"]["state"] = "move-normal"
                elif change == "zero-size":
                    self.native["window"]["clientSizePx"][0] = 0
                else:
                    self.qt["children"][0]["bounds"][0] = float("nan")
                code, report = self.run_tool()
                self.assertEqual(code, 1)
                self.assertIn("invalid-capture", self.codes(report))
                self.qt, self.native = qt, native

    def test_invalid_matrix_has_structured_error_and_exit_two(self):
        self.matrix["cases"] = []
        code, report = self.run_tool()
        self.assertEqual(code, 2)
        self.assertFalse(report["passed"])
        self.assertEqual(report["errors"][0]["code"], "invalid-matrix")

    def test_missing_capture_does_not_stop_other_targets(self):
        self.matrix["cases"][0]["targets"].append({"platform": "linux", "report": "absent.json"})
        code, report = self.run_tool()
        self.assertEqual(code, 1)
        self.assertEqual([r["passed"] for r in report["results"]], [True, False])

    def test_generated_copy_matrix_requires_all_eight_scenarios_and_three_platforms(self):
        path = self.root / "generated.json"
        result = subprocess.run([sys.executable, str(TOOL), "--init-copy-matrix", str(path),
                                 "--reference-exe-sha256", "a" * 64], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)["status"], "capture-required")
        matrix = json.loads(path.read_text())
        self.assertEqual({(c["language"], c["state"]) for c in matrix["cases"]},
                         {(lang, state) for lang in ("en", "ru") for state in
                          ("copy-initial", "copy-compact", "move-initial", "move-compact")})
        result = subprocess.run([sys.executable, str(TOOL), "--matrix", str(path)],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 1)
        report = json.loads(result.stdout)
        self.assertEqual(len(report["results"]), 24)
        self.assertFalse(any(r["passed"] for r in report["results"]))


if __name__ == "__main__":
    unittest.main()
