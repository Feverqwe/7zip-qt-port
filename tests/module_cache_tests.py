"""Compiler-launcher behavior with an object-only cache and explicit MSVC BMIs."""

import importlib.util
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("launcher", ROOT / "scripts/lib/module_cache_launcher.py")
launcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(launcher)


class ModuleCacheTests(unittest.TestCase):
    def test_lost_interface_is_regenerated_alongside_the_object(self):
        with tempfile.TemporaryDirectory(prefix="module outputs ") as directory:
            root = Path(directory)
            bmi, obj = root / "unit.ifc", root / "unit.obj"
            mapping = root / "unit.obj.modmap"
            mapping.write_text(f'-interface\n-ifcOutput "{bmi}"\n')

            def compile_or_restore(arguments, **kwargs):
                obj.write_bytes(b"object")
                if arguments[0] == "compiler":
                    bmi.write_bytes(b"interface")
                return 0

            # A warm object-only cache cannot restore an IFC lost at reboot.
            with patch.object(sys, "argv", ["launcher", "cache", "compiler", "@" + str(mapping)]), \
                    patch.object(launcher.subprocess, "call", side_effect=compile_or_restore):
                for _ in range(2):
                    self.assertEqual(launcher.main(), 0)
                    self.assertEqual(obj.read_bytes(), b"object")
                    self.assertEqual(bmi.read_bytes(), b"interface")
                    bmi.unlink()

    def test_importers_reuse_cache_and_recompile_for_changed_interface(self):
        with tempfile.TemporaryDirectory(prefix="module import ") as directory:
            root = Path(directory)
            bmi, extra, obj = root / "unit with spaces.ifc", root / "user-key", root / "unit.obj"
            bmi.write_bytes(b"first")
            extra.write_bytes(b"retained user input")
            mapping = root / "unit.obj.modmap"
            mapping.write_text(f'\n-reference "unit={bmi}"\n\n')
            cached, compilations = {}, []

            def cached_compile(arguments, env):
                key = tuple(Path(path).read_bytes()
                            for path in env["CCACHE_EXTRAFILES"].split(os.pathsep))
                if key not in cached:
                    compilations.append(key)
                    cached[key] = extra.read_bytes() + bmi.read_bytes()
                obj.write_bytes(cached[key])
                return 0

            with patch.dict(os.environ, {"CCACHE_EXTRAFILES": str(extra)}), \
                    patch.object(sys, "argv", ["launcher", "cache", "compiler", "@" + str(mapping)]), \
                    patch.object(launcher.subprocess, "call", side_effect=cached_compile):
                for _ in range(2):
                    self.assertEqual(launcher.main(), 0)
                    self.assertEqual(obj.read_bytes(), b"retained user inputfirst")
                bmi.write_bytes(b"second")
                self.assertEqual(launcher.main(), 0)
                self.assertEqual(obj.read_bytes(), b"retained user inputsecond")
            self.assertEqual(len(compilations), 2)


if __name__ == "__main__":
    unittest.main()
