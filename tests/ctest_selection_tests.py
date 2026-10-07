#!/usr/bin/env python3
"""Check the project wrapper's slow-test opt-in using actual CTest selection."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class CTestSelectionTests(unittest.TestCase):
    def test_slow_tests_require_opt_in_and_other_exclusions_are_preserved(self):
        with tempfile.TemporaryDirectory(prefix="ctest selection ") as temporary:
            root = Path(temporary)
            (root / "mark.py").write_text("from pathlib import Path\nimport sys\nPath(sys.argv[1]).touch()\n")
            commands = []
            for name in ("fast", "slow", "other"):
                commands.append(f'add_test(NAME {name} COMMAND "{Path(sys.executable).as_posix()}" '
                                f'"{(root / "mark.py").as_posix()}" "{(root / name).as_posix()}")')
            (root / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.21)\nproject(selection NONE)\nenable_testing()\n" +
                "\n".join(commands) + "\nset_tests_properties(slow PROPERTIES LABELS slow)\n" +
                "set_tests_properties(other PROPERTIES LABELS other)\n")
            self.run_command(["cmake", "-S", str(root), "-B", str(root / "build"), "-G", "Ninja"])
            wrapper = ["bash", str(ROOT / "scripts/ctest.sh"), "--test-dir", str(root / "build"),
                       "--output-on-failure"]
            env = dict(os.environ, SEVENZIP_INCLUDE_SLOW_TESTS="0")
            self.run_command([*wrapper, "--label-exclude", "^other$"], env)
            self.assertTrue((root / "fast").exists())
            self.assertFalse((root / "slow").exists())
            self.assertFalse((root / "other").exists())
            self.run_command([*wrapper, "-R", "^slow$"], env)
            self.assertFalse((root / "slow").exists())
            env["SEVENZIP_INCLUDE_SLOW_TESTS"] = "1"
            self.run_command([*wrapper, "-L", "slow"], env)
            self.assertTrue((root / "slow").exists())

    def run_command(self, command, env=None):
        result = subprocess.run(command, env=env, text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
