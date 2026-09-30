import importlib.util
import json
import pathlib
import subprocess
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
        self.source = "CPP/7zip/UI/FileManager/NetFolder.cpp"
        self.exact = "third_party/7zip/CPP/Windows/Net.cpp"
        self.local = "src/CPP/7zip/UI/FileManager/PanelSort.h"
        (self.upstream / self.source).parent.mkdir(parents=True)
        (self.upstream / "CPP/Windows").mkdir(parents=True)
        (self.upstream / self.source).write_text("source body\n")
        (self.upstream / "CPP/Windows/Net.cpp").write_text("exact body\n")
        git(self.upstream, "add", ".")
        git(self.upstream, "commit", "-qm", "baseline")
        self.old = git(self.upstream, "rev-parse", "HEAD")
        (self.port / "third_party/7zip/CPP/Windows").mkdir(parents=True)
        (self.port / "src/CPP/7zip/UI/FileManager").mkdir(parents=True)
        (self.port / self.exact).write_text("exact body\n")
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


if __name__ == "__main__":
    unittest.main()
