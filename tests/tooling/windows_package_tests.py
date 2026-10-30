"""Exercise the actual pre-install verification with disposable runtime bytes."""
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
BASELINE = json.loads((ROOT / "docs/upstream-baseline.json").read_text())


class WindowsPackageVerificationTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="package 空 ")
        self.addCleanup(self.directory.cleanup)
        self.runtime = Path(self.directory.name)
        self.record = {
            "producer": "sevenzip-desktop/build-codec-runtime-windows",
            "revision": BASELINE["revision"],
            "repository": BASELINE["repository"],
            "system": "Windows",
            "architecture": "x86_64",
            "files": {},
        }
        for name in ("7z.exe", "7z.dll"):
            payload = f"disposable {name}".encode()
            (self.runtime / name).write_bytes(payload)
            self.record["files"][name] = {"sha256": hashlib.sha256(payload).hexdigest()}
        template = (ROOT / "cmake/VerifyWindowsRuntime.cmake.in").read_text()
        substitutions = {
            "SEVENZIP_WINDOWS_RUNTIME": self.runtime.as_posix(),
            "sfx_upstream_revision": BASELINE["revision"],
            "windows_runtime_repository": BASELINE["repository"],
            "windows_runtime_architecture": "x86_64",
        }
        for name, value in substitutions.items():
            template = template.replace(f"@{name}@", value)
        self.script = self.runtime / "verify.cmake"
        self.script.write_text(template, encoding="utf-8")

    def verify(self, success):
        (self.runtime / "runtime.json").write_text(json.dumps(self.record), encoding="utf-8")
        result = subprocess.run(["cmake", "-P", str(self.script)], capture_output=True,
                                text=True, timeout=10)
        self.assertEqual(result.returncode == 0, success, result.stdout + result.stderr)

    def test_current_pinned_runtime(self):
        self.verify(True)

    def test_wrong_provenance_and_architecture(self):
        for field in ("producer", "revision", "repository", "system", "architecture"):
            with self.subTest(field=field):
                original = self.record[field]
                self.record[field] = "unrelated"
                self.verify(False)
                self.record[field] = original

    def test_tampered_binary(self):
        (self.runtime / "7z.dll").write_bytes(b"changed")
        self.verify(False)

    def test_missing_binary(self):
        (self.runtime / "7z.exe").unlink()
        self.verify(False)

    def test_missing_recorded_checksum(self):
        del self.record["files"]["7z.exe"]
        self.verify(False)


if __name__ == "__main__":
    unittest.main()
