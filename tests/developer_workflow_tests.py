"""Exercise focused selection from an unbuilt CMake project through actual wrappers."""

import json
import os
from pathlib import Path
import shutil
import re
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]


class WorkflowTests(unittest.TestCase):
    def setUp(self):
        environment = patch.dict(os.environ)
        environment.start()
        self.addCleanup(environment.stop)
        # Each miniature project owns its outputs; inheriting the outer
        # runner's destination would mix fixtures and bypass their cleanup.
        os.environ.pop("SEVENZIP_OUTPUT_ROOT", None)

    def fixture(self, root):
        (root / "scripts/config").mkdir(parents=True)
        (root / "cmake").mkdir()
        (root / "scripts/lib").mkdir(parents=True, exist_ok=True)
        for name in ("build.sh", "ctest.sh", "verify.py", "config/verify-profiles.json",
                     "lib/runner_timing.py", "lib/toolchain.py", "lib/toolchain.sh", "lib/runtime_layout.py",
                     "lib/project_paths.py"):
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

    def test_runner_identity_change_refreshes_native_test_availability(self):
        with tempfile.TemporaryDirectory(prefix="runner selection ") as temporary:
            root = Path(temporary)
            self.fixture(root)
            with (root / "CMakeLists.txt").open("a") as stream:
                stream.write('''
if(NOT "$ENV{SEVENZIP_WINDOWS_RUNNER_HOST}" STREQUAL "" AND
   "$ENV{SEVENZIP_WINDOWS_RUNNER_HOST}" STREQUAL "$ENV{COMPUTERNAME}")
    add_test(NAME configured_native COMMAND subject)
endif()
''')
            with patch.dict(os.environ, {"SEVENZIP_WINDOWS_RUNNER_HOST": "fixture-A",
                                         "COMPUTERNAME": "fixture-A"}):
                self.run_verify(root, "^configured_native$", plan=True)
                os.environ["SEVENZIP_WINDOWS_RUNNER_HOST"] = "fixture-B"
                # A stale configure would still select a test for the wrong host.
                self.run_verify(root, "^configured_native$", plan=True, succeeds=False)

    def test_relocated_outputs_run_and_remain_incremental(self):
        # CTest target discovery must execute persistent binaries even when
        # source and object files are on a separate, disposable filesystem.
        with tempfile.TemporaryDirectory(prefix="split output ") as temporary:
            root = Path(temporary) / "ram"
            root.mkdir()
            self.fixture(root)
            outputs = Path(temporary) / "persistent outputs"
            with patch.dict(os.environ, {"SEVENZIP_OUTPUT_ROOT": str(outputs)}):
                self.run_verify(root, "^scenario$")
                binary = outputs / "dev" / ("subject.exe" if os.name == "nt" else "subject")
                before = binary.stat().st_mtime_ns
                self.run_verify(root, "^scenario$")
                self.assertEqual(binary.stat().st_mtime_ns, before)

    def test_linked_checkout_and_verification_reuse_the_same_objects(self):
        with tempfile.TemporaryDirectory(prefix="linked checkout ") as temporary:
            base = Path(temporary)
            root, alias = base / "source", base / "linked source"
            root.mkdir()
            self.fixture(root)
            if os.name == "nt":
                subprocess.run(["cmd", "/d", "/c", "mklink", "/J", str(alias), str(root)],
                               check=True, capture_output=True)
            else:
                alias.symlink_to(root, target_is_directory=True)
            try:
                result = subprocess.run(
                    ["bash", str(alias / "scripts/build.sh"), "dev", "subject"], cwd=alias,
                    env=dict(os.environ, CMAKE_PREFIX_PATH=str(root)), text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                binary = root / "build/dev" / ("subject.exe" if os.name == "nt" else "subject")
                before = binary.stat().st_mtime_ns
                commands = (root / 'build/dev/compile_commands.json').read_text()
                verified = self.run_verify(root, "^scenario$")
                self.assertEqual(binary.stat().st_mtime_ns, before,
                                 commands + '\nAFTER\n' +
                                 (root / 'build/dev/compile_commands.json').read_text() +
                                 result.stdout + verified.stdout)
            finally:
                if os.name == "nt":
                    alias.rmdir()
                else:
                    alias.unlink()

    def test_unwritable_optional_timing_does_not_fail_verification(self):
        with tempfile.TemporaryDirectory(prefix="optional timing ") as temporary:
            root = Path(temporary)
            self.fixture(root)
            (root / "runner-tmp").write_text("An unavailable timing directory")
            with patch.dict(os.environ, {
                    "SEVENZIP_RUNNER_TIMING": str(root / "runner-tmp/timing.jsonl"),
                    "SEVENZIP_RUN_TOKEN": "fixture", "SEVENZIP_RUNNER_PYTHON": sys.executable}):
                self.run_verify(root, "^scenario$")
            result = json.loads((root / "build/dev/verify-result.json").read_text())
            self.assertEqual(result["status"], "passed")

    def test_analysis_mode_rechecks_an_unchanged_incremental_build(self):
        # Enabling analysis must run its diagnostics even when the executable
        # was already built, and disabling it must recover the same build tree.
        with tempfile.TemporaryDirectory(prefix="analysis workflow ") as temporary:
            root = Path(temporary)
            self.fixture(root)
            (root / "analyze.py").write_text('raise SystemExit("analysis rejected fixture")\n')
            python = Path(sys.executable).as_posix()
            (root / "CMakeLists.txt").write_text('''cmake_minimum_required(VERSION 3.21)
project(analysis_workflow LANGUAGES CXX)
option(SEVENZIP_ENABLE_CLANG_TIDY "Analyze the fixture" OFF)
if(SEVENZIP_ENABLE_CLANG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY "''' + python + ''';${CMAKE_SOURCE_DIR}/analyze.py")
endif()
enable_testing()
add_executable(subject main.cpp)
add_test(NAME scenario COMMAND subject)
include(cmake/DeveloperWorkflow.cmake)
''')
            with patch.dict(os.environ, {"SEVENZIP_ENABLE_CLANG_TIDY": "OFF"}):
                self.run_verify(root, "^scenario$")
                os.environ["SEVENZIP_ENABLE_CLANG_TIDY"] = "ON"
                rejected = self.run_verify(root, "^scenario$", succeeds=False)
                self.assertIn("analysis rejected fixture", rejected.stdout + rejected.stderr)
                os.environ["SEVENZIP_ENABLE_CLANG_TIDY"] = "OFF"
                self.run_verify(root, "^scenario$")

    def test_clang_analysis_imports_a_module_and_rejects_a_dangling_view(self):
        compiler = shutil.which(os.environ.get("CXX", "clang++"))
        if not compiler:
            self.skipTest("Clang module toolchain unavailable")
        version = subprocess.run([compiler, "--version"], text=True, capture_output=True)
        match = re.search(r"clang version (\d+)", version.stdout)
        if not match or int(match[1]) < 19:
            self.skipTest("Clang 19 or newer required")
        analyzer = (shutil.which("clang-tidy-" + match[1]) or
                    shutil.which("clang-tidy", path=str(Path(compiler).parent)))
        if not analyzer:
            self.skipTest("Matching clang-tidy unavailable")
        with tempfile.TemporaryDirectory(prefix="module analysis ") as temporary:
            root = Path(temporary)
            self.fixture(root)
            shutil.copy2(ROOT / "cmake/StaticAnalysis.cmake", root / "cmake/StaticAnalysis.cmake")
            shutil.copy2(ROOT / ".clang-tidy", root / ".clang-tidy")
            shutil.copy2(ROOT / "scripts/lib/clang_tidy_launcher.py",
                         root / "scripts/lib/clang_tidy_launcher.py")
            (root / "answer.cppm").write_text(
                "export module answer;\nexport int answer() { return 42; }\n")
            (root / "main.cpp").write_text("import answer;\nint main() { return answer() == 42 ? 0 : 1; }\n")
            (root / "CMakeLists.txt").write_text('''cmake_minimum_required(VERSION 3.28)
project(module_analysis LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(SEVENZIP_CLANG_TIDY_EXECUTABLE "''' + Path(analyzer).as_posix() + '''" CACHE FILEPATH "")
include(cmake/StaticAnalysis.cmake)
enable_testing()
add_executable(subject main.cpp)
target_sources(subject PRIVATE FILE_SET CXX_MODULES FILES answer.cppm)
sevenzip_track_analysis_inputs()
add_test(NAME scenario COMMAND subject)
include(cmake/DeveloperWorkflow.cmake)
''')
            with patch.dict(os.environ, {"CC": compiler, "CXX": compiler,
                                         "SEVENZIP_ENABLE_CLANG_TIDY": "ON"}):
                self.run_verify(root, "^scenario$")
                (root / "main.cpp").write_text(
                    '#include <string>\n#include <string_view>\nimport answer;\n'
                    'std::string_view dangling() { return std::string("temporary"); }\n'
                    'int main() { return answer() == 42 ? 0 : 1; }\n')
                rejected = self.run_verify(root, "^scenario$", succeeds=False)
                self.assertIn("clang-diagnostic-return-stack-address", rejected.stdout + rejected.stderr)

    def test_analysis_launcher_preserves_quoted_module_paths(self):
        # Windows response parsing must also treat line breaks as separators;
        # spaces inside BMI paths must reach the tooling driver unchanged.
        with tempfile.TemporaryDirectory(prefix="module paths Юникод ") as temporary:
            root = Path(temporary)
            (root / "clang.modmap").write_text(
                '-fmodule-file=answer="module with spaces.pcm"\n'
                '-fmodule-output="compiled module.pcm"\n')
            (root / "msvc.modmap").write_text('-reference answer=answer.ifc\n')
            (root / "capture.py").write_text(
                'import json,sys\nfrom pathlib import Path\n'
                'Path("arguments.json").write_text(json.dumps(sys.argv[1:]))\n')
            subprocess.run([sys.executable, str(ROOT / "scripts/lib/clang_tidy_launcher.py"),
                            sys.executable, "capture.py", "--", "@clang.modmap", "@msvc.modmap"],
                           cwd=root, check=True)
            self.assertEqual(json.loads((root / "arguments.json").read_text()),
                             ["--", "-fmodule-file=answer=module with spaces.pcm",
                              "-fmodule-output=compiled module.pcm", "@msvc.modmap"])

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
        return result

if __name__ == "__main__":
    unittest.main()
