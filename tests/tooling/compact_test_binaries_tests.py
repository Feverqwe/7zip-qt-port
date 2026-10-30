#!/usr/bin/env python3
"""Verify compact links retain Debug behavior and production symbols."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]


class CompactTestBinariesTests(unittest.TestCase):
    @unittest.skipUnless(sys.platform in ("linux", "win32"), "Linux/MSVC linker policy")
    def test_compact_links_keep_debug_behavior_and_application_symbols(self):
        with tempfile.TemporaryDirectory(prefix="compact link ") as temporary:
            root = Path(temporary)
            (root / "probe.cpp").write_text("""#include <cassert>
#ifdef NDEBUG
#error Compact test binaries must retain Debug assertions
#endif
int first() { return 7; }
int second() { return 7; }
int main() {
    assert(first() == second());
    assert(&first != &second);
}
""")
            module = (ROOT / "cmake/CompactTestBinaries.cmake").as_posix()
            (root / "CMakeLists.txt").write_text(f"""
cmake_minimum_required(VERSION 3.21)
project(compact_link_probe LANGUAGES CXX)
set(BUILD_TESTING ON)
add_executable(sevenzip_desktop probe.cpp)
add_executable(compact_probe probe.cpp)
include("{module}")
""")
            for mode in ("OFF", "ON"):
                with self.subTest(mode=mode):
                    build = root / mode
                    self.run_command("cmake", "-S", str(root), "-B", str(build),
                                     "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Debug",
                                     "-DSEVENZIP_COMPACT_TEST_BINARIES=" + mode)
                    self.run_command("cmake", "--build", str(build))
                    suffix = ".exe" if os.name == "nt" else ""
                    for name in ("sevenzip_desktop", "compact_probe"):
                        self.run_command(str(build / (name + suffix)))
                    if os.name == "nt":
                        self.assertTrue((build / "sevenzip_desktop.pdb").is_file())
                        self.assertEqual((build / "compact_probe.pdb").exists(), mode == "OFF")
                        self.assertEqual((build / "compact_probe.ilk").exists(), mode == "OFF")
                    else:
                        app_sections = self.run_command("readelf", "-SW", str(build / "sevenzip_desktop"))
                        test_sections = self.run_command("readelf", "-SW", str(build / "compact_probe"))
                        self.assertIn(".debug_info", app_sections)
                        self.assertEqual(".debug_info" in test_sections, mode == "OFF")

    def run_command(self, *command):
        result = subprocess.run(command, text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return result.stdout


if __name__ == "__main__":
    unittest.main()
