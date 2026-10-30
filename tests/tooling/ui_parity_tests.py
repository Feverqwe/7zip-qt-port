"""Deterministic resource/contract tests; no sibling checkout or Qt required."""
import copy
import json
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from check_ui_parity import compare
from lib.ui_reference import behavior_symbols, parse_dialogs, preprocess


class ResourceTests(unittest.TestCase):
    def test_handler_and_helper_definition_index_excludes_declarations(self):
        source = '''
bool CDialog::OnInit() { return true; }
void CDialog::UpdateStatInfo(bool showAll) { SetItemText(1, value); }
void CDialog::SetPauseText() { }
bool CDialog::HasErrors() const { return true; }
void CDialog::Unimplemented();
void PlainFunction() { }
'''
        self.assertEqual(behavior_symbols(source), [
            "CDialog::HasErrors", "CDialog::OnInit", "CDialog::SetPauseText",
            "CDialog::UpdateStatInfo"])

    def test_macros_includes_conditionals_and_symbolic_ids(self):
        compiler = shutil.which("clang") or shutil.which("gcc")
        if not compiler:
            self.skipTest("Extraction requires clang/gcc; comparison does not")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "ids.h").write_text("#define IDD_TEST 100\n#define IDC_PATH 101\n",
                                      encoding="utf-8")
            (root / "common.rc").write_text(
                '#include <windows.h>\n#define X 8\n#define W (100 + X)\n'
                '#define BUTTON(t,id,x) PUSHBUTTON t,id,x,40,30,12\n', encoding="utf-8")
            resource = '#include "ids.h"\n#include "common.rc"\n'
            resource += 'IDD_TEST DIALOG 0,0,W,80 STYLE WS_POPUP FONT 8,"Font"\n'
            resource += 'CAPTION "Unicode окно"\nBEGIN\n'
            resource += 'COMBOBOX IDC_PATH,X,10,W-X,70,CBS_DROPDOWN | WS_TABSTOP\n'
            resource += 'BUTTON("OK",IDOK,20)\n'
            resource += '#ifdef UNDER_CE\nLTEXT "CE",IDC_CE,0,0,10,10\n#endif\nEND\n'
            (root / "dialog.rc").write_text(resource, encoding="utf-8")
            text, dependencies, opaque = preprocess(root, "dialog.rc", compiler)
            dialog = parse_dialogs(text)[0]
            self.assertEqual(dialog["id"], "IDD_TEST")
            self.assertEqual(dialog["boundsDLU"], [0, 0, 108, 80])
            self.assertEqual(dialog["caption"], "Unicode окно")
            self.assertEqual([c["id"] for c in dialog["controls"]], ["IDC_PATH", "IDOK"])
            self.assertTrue(dialog["controls"][0]["editable"])
            self.assertEqual(dialog["controls"][0]["boundsDLU"], [8, 10, 100, 70])
            self.assertEqual(set(dependencies), {"dialog.rc", "common.rc", "ids.h"})
            self.assertEqual(opaque, ["windows.h"])

    def test_unknown_statements_and_expressions_fail_closed(self):
        for statement in ['SCROLLBAR IDC_UNKNOWN,0,0,10,10',
                          'LTEXT "a",IDC_A,unknown,0,10,10',
                          'LTEXT "a",IDC_A,__import__("os"),0,10,10']:
            with self.subTest(statement=statement), self.assertRaises((ValueError, SyntaxError)):
                parse_dialogs('IDD_TEST DIALOG 0,0,100,80 BEGIN ' + statement + ' END')

    def test_control_macro_style_and_multiple_dialogs(self):
        dialogs = parse_dialogs('''
            IDD_A DIALOG 0,0,100,80 BEGIN
            CONTROL "Show",IDC_SHOW,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,8,8,80,10
            COMBOBOX IDC_MODE,8,22,80,70,CBS_DROPDOWNLIST
            END
            IDD_B DIALOGEX 0,0,100,80 { EDITTEXT IDE_VALUE,8,8,80,12,ES_PASSWORD }
        ''')
        self.assertEqual(len(dialogs), 2)
        self.assertEqual(dialogs[0]["controls"][0]["qtClass"], "QCheckBox")
        self.assertFalse(dialogs[0]["controls"][1]["editable"])
        self.assertIn("ES_PASSWORD", dialogs[1]["controls"][0]["style"])

    def test_icons_do_not_consume_neighboring_resource_statements(self):
        dialog = parse_dialogs('''IDD_TEST DIALOG 0,0,100,80 BEGIN
            LTEXT "File",IDT_FILE,8,8,80,8
            ICON "",IDI_FILE,8,20 + 4,24,24
            LTEXT "Size",IDT_SIZE,40,24,50,8
            END''')[0]
        self.assertEqual([c["id"] for c in dialog["controls"]],
                         ["IDT_FILE", "IDI_FILE", "IDT_SIZE"])
        self.assertEqual(dialog["controls"][1]["boundsDLU"], [8,24,24,24])
        self.assertEqual(dialog["controls"][1]["qtClass"], "QLabel")


class CompareTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.reference = {
            "schema": 1, "upstreamCommit": "abc", "dependencies": {"Dialog.cpp": "hash"},
            "behaviorSymbols": {"Dialog.cpp": ["CDialog::OnInit"]},
            "dialogs": parse_dialogs('''IDD_TEST DIALOG 0,0,100,80 BEGIN
                LTEXT "Path",IDT_PATH,8,8,80,8
                COMBOBOX IDC_PATH,8,20,80,70,CBS_DROPDOWN
                END''')}
        self.contract = {
            "schema": 1, "upstreamCommit": "abc", "dependencies": {"Dialog.cpp": "hash"},
            "dialog": "IDD_TEST", "controls": {
                "IDT_PATH": {"selector": {"name": "label"}},
                "IDC_PATH": {"selector": {"name": "path"}}},
            "relations": [{"a": "IDT_PATH", "b": "IDC_PATH", "kind": "above"}],
            "states": [{"name": "normal", "reports": ["normal.json"],
                        "evidence": [{"source": "Dialog.cpp", "symbol": "CDialog::OnInit"}],
                        "assert": [{"id": "IDC_PATH", "property": "enabled", "equals": True}]}]}
        self.report = {"class": "QDialog", "children": [
            {"name": "label", "class": "QLabel", "text": "Path", "bounds": [8, 8, 80, 8]},
            {"name": "path", "class": "QComboBox", "editable": True,
             "enabled": True, "bounds": [8, 20, 80, 20]}]}

    def check(self):
        Path(self.temp.name, "normal.json").write_text(json.dumps(self.report), encoding="utf-8")
        return compare(self.reference, self.contract, self.temp.name)

    def test_matching_report_passes(self):
        self.assertTrue(self.check()["passed"])

    def test_ui_regressions_fail(self):
        for field, value in [("enabled", False), ("editable", False),
                             ("class", "QLineEdit"), ("bounds", [8, 0, 80, 20])]:
            with self.subTest(field=field):
                original = copy.deepcopy(self.report)
                self.report["children"][1][field] = value
                self.assertFalse(self.check()["passed"])
                self.report = original

    def test_missing_ambiguous_and_unmapped_controls_fail(self):
        self.report["children"].append(copy.deepcopy(self.report["children"][1]))
        self.assertFalse(self.check()["passed"])
        self.report["children"] = self.report["children"][:1]
        self.assertFalse(self.check()["passed"])
        del self.contract["controls"]["IDC_PATH"]
        self.assertIn("Unreviewed upstream control: IDC_PATH", self.check()["errors"])

    def test_missing_state_reports_fail(self):
        result = compare(self.reference, self.contract, self.temp.name)
        self.assertFalse(result["passed"])
        self.assertEqual(result["checked"], [])

    def test_baseline_change_and_invented_evidence_rejected(self):
        self.contract["dependencies"]["Dialog.cpp"] = "changed"
        with self.assertRaises(ValueError):
            self.check()
        self.contract["dependencies"]["Dialog.cpp"] = "hash"
        self.contract["states"][0]["evidence"][0]["symbol"] = "CDialog::Invented"
        with self.assertRaises(ValueError):
            self.check()

    def test_unsupported_resource_relation_rejected(self):
        self.contract["relations"][0]["kind"] = "left_of"
        with self.assertRaises(ValueError):
            self.check()

    def test_adaptations_are_explicit(self):
        self.report["children"][0]["text"] = "Путь"
        self.assertFalse(self.check()["passed"])
        self.contract["controls"]["IDT_PATH"]["textAdaptation"] = "Russian translation"
        self.assertTrue(self.check()["passed"])
        self.assertTrue(self.check()["deviations"])

    def test_conditional_visibility_requires_both_state_reports(self):
        self.report["children"][1]["visible"] = False
        hidden = copy.deepcopy(self.contract["states"][0])
        hidden.update(name="field-hidden", reports=["hidden.json"])
        hidden["assert"] = [{"id": "IDC_PATH", "property": "visible", "equals": False}]
        self.contract["states"].append(hidden)
        Path(self.temp.name, "hidden.json").write_text(json.dumps(self.report), encoding="utf-8")
        self.report["children"][1]["visible"] = True
        self.contract["states"][0]["assert"].append(
            {"id": "IDC_PATH", "property": "visible", "equals": True})
        self.assertTrue(self.check()["passed"])
        # A broken toggle leaves the field visible in the supposedly hidden state.
        Path(self.temp.name, "hidden.json").write_text(json.dumps(self.report), encoding="utf-8")
        self.assertFalse(self.check()["passed"])


if __name__ == "__main__":
    unittest.main()
