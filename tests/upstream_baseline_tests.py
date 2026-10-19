import importlib.util
import json
import pathlib
import os
import shutil
import subprocess
import sys
import time
import tempfile
import unittest


SCRIPT = pathlib.Path(__file__).resolve().parents[1] / "scripts/upstream_baseline.py"
spec = importlib.util.spec_from_file_location("upstream_baseline", SCRIPT)
baseline = importlib.util.module_from_spec(spec)
spec.loader.exec_module(baseline)


def git(repo, *args):
    return subprocess.check_output(["git", "-C", str(repo), *args], stderr=subprocess.DEVNULL).decode().strip()


class UpstreamBaselineTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        root = pathlib.Path(self.temporary.name)
        self.port = root / "port"
        self.upstream = root / "upstream"
        self.port.mkdir()
        self.upstream.mkdir()
        for repo in (self.port, self.upstream):
            git(repo, "init", "-q")
            git(repo, "config", "user.name", "Fixture")
            git(repo, "config", "user.email", "fixture@example.invalid")
            # Exact-byte fixtures must not inherit the host Git newline policy.
            git(repo, "config", "core.autocrlf", "false")
        self.source = "CPP/7zip/UI/FileManager/NetFolder.cpp"
        self.exact = "third_party/7zip/CPP/Windows/Net.cpp"
        self.local = "src/CPP/7zip/UI/FileManager/PanelSort.h"
        (self.upstream / self.source).parent.mkdir(parents=True)
        (self.upstream / "CPP/Windows").mkdir(parents=True)
        (self.upstream / self.source).write_text("source body\n")
        (self.upstream / "CPP/Windows/Net.cpp").write_bytes(b"exact body\n")
        git(self.upstream, "add", ".")
        git(self.upstream, "commit", "-qm", "baseline")
        self.old = git(self.upstream, "rev-parse", "HEAD")
        (self.port / "third_party/7zip/CPP/Windows").mkdir(parents=True)
        (self.port / "src/CPP/7zip/UI/FileManager").mkdir(parents=True)
        (self.port / self.exact).write_bytes(b"exact body\n")
        (self.port / ("src/" + self.source)).write_text("source body with Qt adapter\n")
        (self.port / self.local).write_text("local declaration\n")
        git(self.port, "add", ".")
        git(self.port, "commit", "-qm", "port")
        self.manifest = {
            "schema": 1,
            "repository": "https://github.com/ip7z/7zip.git",
            "revision": self.old,
            "exact": [self.exact],
            "adapted": ["src/" + self.source],
            "local": [self.local],
        }
        self.manifest_path = root / "manifest.json"
        self.manifest_path.write_text(json.dumps(self.manifest))
        old_root = baseline.ROOT
        baseline.ROOT = self.port
        self.addCleanup(setattr, baseline, "ROOT", old_root)

    def test_inventory_and_exact_bytes(self):
        data = baseline.load_manifest(self.manifest_path)
        baseline.verify(data, self.upstream)
        (self.port / self.exact).write_text("changed vendor\n")
        with self.assertRaisesRegex(ValueError, "Exact vendor differs"):
            baseline.verify(data, self.upstream)

    def test_review_reports_adapted_and_exact_changes(self):
        (self.upstream / self.source).write_text("upstream revised source\n")
        (self.upstream / "CPP/Windows/Net.cpp").write_text("upstream revised vendor\n")
        git(self.upstream, "add", ".")
        git(self.upstream, "commit", "-qm", "proposal")
        new = git(self.upstream, "rev-parse", "HEAD")
        data = baseline.load_manifest(self.manifest_path)
        baseline.verify(data, self.upstream)
        report = baseline.review(data, self.upstream, new, [])
        self.assertIn(self.exact + "` (exact", report)
        self.assertIn("src/" + self.source + "` (adapted", report)
        self.assertIn("`" + self.old + "` → `" + new + "`", report)

    def test_source_move_can_be_verified_before_staging(self):
        destination = "src/CPP/Windows/Net.cpp"
        (self.port / destination).parent.mkdir(parents=True)
        (self.port / self.exact).rename(self.port / destination)
        self.manifest["exact"] = []
        self.manifest["adapted"] = sorted([*self.manifest["adapted"], destination])
        self.manifest_path.write_text(json.dumps(self.manifest))
        data = baseline.load_manifest(self.manifest_path)
        baseline.verify(data, self.upstream)
        # New worktree files still require provenance.
        data["adapted"].remove(destination)
        with self.assertRaisesRegex(ValueError, "Inventory mismatch: unrecorded="):
            baseline.verify(data, self.upstream)

    def test_runtime_snapshot_updates_selected_sources_and_retains_incremental_outputs(self):
        snapshot = pathlib.Path(self.temporary.name) / "runtime"
        self.manifest["runtime_adapted"] = ["src/" + self.source]
        baseline.prepare_runtime_snapshot(self.manifest, snapshot, self.upstream)
        adapted = snapshot / self.source
        self.assertEqual(adapted.read_bytes(), (self.port / ("src/" + self.source)).read_bytes())
        self.assertEqual((snapshot / "CPP/Windows/Net.cpp").read_bytes(), b"exact body\n")
        compiled = snapshot / "CPP/7zip/output.o"
        compiled.write_bytes(b"cached compiler output")
        before = adapted.stat().st_mtime_ns
        baseline.prepare_runtime_snapshot(self.manifest, snapshot, self.upstream)
        self.assertEqual(adapted.stat().st_mtime_ns, before)
        (self.port / ("src/" + self.source)).write_bytes(b"new adapted input")
        baseline.prepare_runtime_snapshot(self.manifest, snapshot, self.upstream)
        self.assertEqual(adapted.read_bytes(), b"new adapted input")
        self.assertEqual(compiled.read_bytes(), b"cached compiler output")

    def test_runtime_snapshot_refuses_unowned_directory(self):
        snapshot = pathlib.Path(self.temporary.name) / "runtime"
        snapshot.mkdir()
        private = snapshot / "user-file"
        private.write_bytes(b"user data")
        with self.assertRaisesRegex(ValueError, "unowned"):
            baseline.prepare_runtime_snapshot(self.manifest, snapshot, self.upstream)
        self.assertEqual(private.read_bytes(), b"user data")

    def test_runtime_rebuilds_transitive_adapted_header_consumers(self):
        directory = "CPP/7zip/UI/Console"
        header = "CPP/7zip/Common/Model.h"
        files = {
            directory + "/Probe.cpp": '#include "../../Common/Bridge.h"\nint main() { return sizeof(Model); }\n',
            "CPP/7zip/Common/Bridge.h": '#include "Model.h"\n',
            header: "struct Model { char value; };\n",
        }
        windows = sys.platform == "win32"
        cache = shutil.which("ccache")
        self.assertIsNotNone(cache, "Runtime compilation must use the existing compiler cache")
        compiler = '"' + cache + '" ' + ("cl /nologo" if windows else "c++")
        if windows:
            makefile = ('O=objects\nall: $(O)\\probe.exe\n'
                        '$(O)\\probe.exe: $(O)\\Probe.obj\n'
                        f'\t{compiler} /Fe$@ $(O)\\Probe.obj\n'
                        '$(O)\\Probe.obj: Probe.cpp\n'
                        f'\t{compiler} /c /Fo$@ Probe.cpp\n')
        else:
            makefile = ('O=objects\nall: $(O)/probe\n'
                        '$(O)/probe: $(O)/Probe.o\n'
                        f'\t{compiler} -o $@ $<\n'
                        '$(O)/Probe.o: Probe.cpp\n'
                        f'\t{compiler} -c -o $@ $<\n')
        files[directory + ("/makefile" if windows else "/makefile.gcc")] = makefile
        for name, contents in files.items():
            path = self.upstream / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(contents)
        git(self.upstream, "add", ".")
        git(self.upstream, "commit", "-qm", "native dependency fixture")
        self.manifest["revision"] = git(self.upstream, "rev-parse", "HEAD")
        local = "src/" + header
        (self.port / local).parent.mkdir(parents=True, exist_ok=True)
        (self.port / local).write_text(files[header])
        self.manifest["adapted"] = sorted([*self.manifest["adapted"], local])
        self.manifest["runtime_adapted"] = [local]
        snapshot = pathlib.Path(self.temporary.name) / "runtime"
        environment = dict(os.environ)
        if sys.platform == "darwin":
            environment.setdefault("CCACHE_DIR", "/private/tmp/sevenzip-desktop-ccache")
        command = ["nmake", "/nologo", "/f", "sevenzip-runtime.mak"] if windows else [
            "make", "-f", "sevenzip-runtime.gcc.mak"]
        for contents, expected in ((files[header], 1), ("struct Model { long long value; };\n", 8)):
            # Apple GNU make 3.81 and nmake compare whole-second timestamps.
            if expected != 1:
                time.sleep(1.1)
            (self.port / local).write_text(contents)
            baseline.prepare_runtime_snapshot(self.manifest, snapshot, self.upstream)
            bundle = snapshot / directory
            (bundle / "objects").mkdir(exist_ok=True)
            compiled = subprocess.run(command, cwd=bundle, env=environment, capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
            executable = bundle / "objects" / ("probe.exe" if windows else "probe")
            checked = executable
            if windows:
                # Keep the relink target out of Windows' executable image locks.
                checked = executable.with_name("probe-checked-" + str(expected) + ".exe")
                shutil.copy2(executable, checked)
            self.assertEqual(subprocess.run([str(checked)]).returncode, expected)


if __name__ == "__main__":
    unittest.main()
