"""Exercise focused selection from an unbuilt CMake project through actual wrappers."""

import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]


class WorkflowTests(unittest.TestCase):
    def fixture(self, root):
        (root / "scripts").mkdir()
        (root / "cmake").mkdir()
        for name in ("build.sh", "ctest.sh", "verify.py", "verify-profiles.json"):
            shutil.copy2(ROOT / "scripts" / name, root / "scripts" / name)
        shutil.copy2(ROOT / "cmake/DeveloperWorkflow.cmake", root / "cmake/DeveloperWorkflow.cmake")
        shutil.copy2(ROOT / "cmake/ReadCTestCommands.cmake", root / "cmake/ReadCTestCommands.cmake")
        shutil.copy2(ROOT / "CMakePresets.json", root / "CMakePresets.json")
        (root / "main.cpp").write_text("int main() { return 0; }\n")
        (root / "CMakeLists.txt").write_text('''cmake_minimum_required(VERSION 3.21)
project(workflow LANGUAGES CXX)
file(APPEND "${CMAKE_BINARY_DIR}/configure-count" "configured\n")
enable_testing()
add_executable(subject main.cpp)
add_executable(producer main.cpp)
add_executable(unrelated main.cpp)
add_test(NAME scenario COMMAND subject)
add_test(NAME setup COMMAND producer)
add_test(NAME long COMMAND unrelated)
set_tests_properties(setup PROPERTIES FIXTURES_SETUP input)
set_tests_properties(scenario PROPERTIES FIXTURES_REQUIRED input)
set_tests_properties(long PROPERTIES LABELS slow)
include(cmake/DeveloperWorkflow.cmake)
''')

    def test_first_build_includes_fixture_and_preserves_incremental_output(self):
        with tempfile.TemporaryDirectory(prefix="verify Юникод ") as temporary:
            root = Path(temporary)
            self.fixture(root)
            with patch.dict(os.environ, {"SEVENZIP_INCLUDE_SLOW_TESTS": "0"}):
                self.run_verify(root, "^scenario$")
                report = root / "build/dev/verify-result.json"
                result = json.loads(report.read_text())
                self.assertEqual(result["status"], "passed")
                suffix = ".exe" if os.name == "nt" else ""
                binary = root / ("build/dev/subject" + suffix)
                before = binary.stat().st_mtime_ns
                self.assertFalse((root / ("build/dev/unrelated" + suffix)).exists())
                self.run_verify(root, "^scenario$")
                self.assertEqual(before, binary.stat().st_mtime_ns)
                self.assertEqual((root / "build/dev/configure-count").read_text().splitlines(), ["configured"])
                with (root / "CMakeLists.txt").open("a") as stream:
                    stream.write("add_test(NAME added COMMAND subject)\n")
                self.run_verify(root, "^added$", plan=True)
                planned = json.loads((root / "build/dev/verify-plan.json").read_text())
                self.assertEqual(planned["status"], "planned")
                self.assertEqual(json.loads(report.read_text())["status"], "passed")
                self.assertEqual(len((root / "build/dev/configure-count").read_text().splitlines()), 2)
                self.run_verify(root, "^does_not_exist$", succeeds=False)
                self.assertEqual(json.loads(report.read_text())["status"], "failed")
                self.run_verify(root, "^long$", succeeds=False)

    def run_verify(self, root, selection, plan=False, succeeds=True):
        command = [sys.executable, str(root / "scripts/verify.py"), "--tests", selection, "--skip-runtime"]
        if plan:
            command.append("--plan")
        # This fixture exercises the wrappers without any Qt dependency.
        # Supply its own prefix instead of requiring a machine-local SDK.
        environment = dict(os.environ, CMAKE_PREFIX_PATH=str(root))
        result = subprocess.run(command, cwd=root, env=environment, text=True, capture_output=True)
        if succeeds:
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        else:
            self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)

if __name__ == "__main__":
    unittest.main()
