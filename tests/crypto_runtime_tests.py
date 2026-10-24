"""Compile a real consumer of the exported native dependency configuration."""
import importlib.util
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import tempfile
import time
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "scripts/crypto_provider.py"
spec = importlib.util.spec_from_file_location("crypto_provider_tools", SCRIPT)
crypto = importlib.util.module_from_spec(spec)
spec.loader.exec_module(crypto)


class CryptoRuntimeTests(unittest.TestCase):
    def test_compiler_flags_headers_linking_and_dependency_changes_reach_executable(self):
        with tempfile.TemporaryDirectory(prefix="sevenzip crypto ") as temporary:
            root = Path(temporary)
            (root / "build").mkdir()
            dependency = root / "external library"
            dependency.mkdir()
            (dependency / "openssl").mkdir()
            header = dependency / "openssl/contract.h"
            header.write_text('extern "C" int external_value(void);\n#define HEADER_VALUE 0\n')
            source = dependency / "provider.c"
            windows = os.name == "nt"
            cache = shutil.which("ccache")
            self.assertIsNotNone(cache, "Use the existing compiler cache")
            compiler = "cl" if windows else "cc"
            library = dependency / ("provider.lib" if windows else "libprovider.a")
            obj = dependency / ("provider.obj" if windows else "provider.o")
            env = os.environ.copy()
            if windows:
                # Git Bash supplies a different link.exe. Keep the compiler's
                # tool directory first, as in the official native builder.
                env["PATH"] = str(Path(shutil.which("cl")).parent) + os.pathsep + env["PATH"]
            if platform.system() == "Darwin":
                env.setdefault("CCACHE_DIR", "/private/tmp/sevenzip-desktop-ccache")

            def run(args, cwd=None):
                result = subprocess.run(args, cwd=cwd, env=env, capture_output=True)
                self.assertEqual(result.returncode, 0, (result.stdout + result.stderr).decode(errors="replace"))
                return result

            def build_library(value):
                source.write_text("int external_value(void) { return " + str(value) + "; }\n")
                if windows:
                    run([cache, compiler, "/nologo", "/c", "/Fo" + str(obj), str(source)])
                    run(["lib", "/nologo", "/OUT:" + str(library), str(obj)])
                else:
                    run([cache, compiler, "-c", "-o", str(obj), str(source)])
                    run(["ar", "rcs", str(library), str(obj)])

            build_library(2)
            snapshot = root / "runtime"
            bundle = snapshot / "CPP/7zip/UI/Console"
            (bundle / "objects").mkdir(parents=True)
            assembler = windows and platform.machine().lower() in ("amd64", "x86_64")
            (bundle / "Probe.cpp").write_text(
                '#include "openssl/contract.h"\n' +
                ('extern "C" int asm_value(void);\n' if assembler else '') +
                'int main() { return external_value() + BACKEND_VALUE + HEADER_VALUE' +
                (' + asm_value()' if assembler else '') + '; }\n')
            if windows:
                (snapshot / "CPP/Build.mak").write_text(
                    f'COMPILE = "{cache}" cl $(CFLAGS) $**\n'
                    'ASSEMBLE = ml64 -nologo -c -Fo$(O)/ $**\n')
                if assembler:
                    (bundle / "zero.asm").write_text(
                        '.code\nPUBLIC asm_value\nasm_value PROC\nxor eax,eax\nret\nasm_value ENDP\nEND\n')
                makefile = ('O=objects\nPROGPATH=$(O)\\probe.exe\nOBJS=$(O)\\Probe.obj\n'
                            + ('OBJS=$(OBJS) $(O)\\zero.obj\n' if assembler else '') +
                            'CFLAGS=$(CFLAGS) /nologo /c\n!include "../../../Build.mak"\nall: $(PROGPATH)\n'
                            '$(PROGPATH): $(OBJS)\n'
                            '\tlink /nologo /OUT:$@ $(OBJS) $(LIBS)\n'
                            '$(O)\\Probe.obj: Probe.cpp\n'
                            '\t$(COMPILE) /Fo$@\n' +
                            ('$(O)\\zero.obj: zero.asm\n\t$(ASSEMBLE)\n' if assembler else ''))
            else:
                makefile = ('O=objects\nPROGPATH=$(O)/probe\nOBJS=$(O)/Probe.o\n'
                            'all: $(PROGPATH)\n$(PROGPATH): $(OBJS)\n'
                            f'\t"{cache}" c++ -o $@ $(OBJS) $(MY_LIBS)\n'
                            '$(O)/Probe.o: Probe.cpp\n'
                            f'\t"{cache}" c++ $(LOCAL_FLAGS) -c -o $@ Probe.cpp\n')
            original = "makefile" if windows else "makefile.gcc"
            generated = "sevenzip-runtime.mak" if windows else "sevenzip-runtime.gcc.mak"
            (bundle / original).write_text(makefile)
            (bundle / generated).write_text(('!include "' + original + '"' if windows else
                                             'include ' + original) + "\n")
            config = {"schema": 1, "provider": "OPENSSL", "version": "fixture",
                      "system": platform.system(), "deployment_target": "",
                      "definitions": ["BACKEND_VALUE=3"], "includes": [str(dependency)],
                      "libraries": [str(library)]}

            def build_and_result():
                (root / "build/crypto-provider.json").write_text(json.dumps(config))
                crypto.stage(root, snapshot)
                run(["nmake", "/nologo", "/f", generated] if windows else
                    ["make", "-s", "-f", generated], bundle)
                executable = bundle / "objects" / ("probe.exe" if windows else "probe")
                return subprocess.run([str(executable)], env=env).returncode

            self.assertEqual(build_and_result(), 5)
            object_path = bundle / "objects" / ("Probe.obj" if windows else "Probe.o")
            before = object_path.stat().st_mtime_ns
            self.assertEqual(build_and_result(), 5)
            self.assertEqual(before, object_path.stat().st_mtime_ns)
            time.sleep(1.1)  # Native make variants can use whole-second timestamps.
            config["definitions"] = ["BACKEND_VALUE=7"]
            self.assertEqual(build_and_result(), 9)
            time.sleep(1.1)
            build_library(4)
            self.assertEqual(build_and_result(), 11)
            time.sleep(1.1)
            header.write_text('extern "C" int external_value(void);\n#define HEADER_VALUE 1\n')
            self.assertEqual(build_and_result(), 12)


if __name__ == "__main__":
    unittest.main()
