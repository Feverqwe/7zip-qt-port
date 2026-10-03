"""Offline checks for runner snapshots and failure records; no VM access."""

import hashlib
import importlib.util
import json
import io
from pathlib import Path
import subprocess
import tarfile
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location(
    "hyperv_runner", Path(__file__).resolve().parents[1] / "scripts/hyperv-runner.py")
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


def git(directory, *args):
    return subprocess.check_output(["git", "-C", str(directory), *args], text=True).strip()


class RunnerTests(unittest.TestCase):
    def test_linux_preparation_failure_keeps_exit_code_and_prints_log_once(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            archive_path = root / "source.tar.gz"
            with tarfile.open(archive_path, "w:gz") as archive:
                for name, contents in (("README.md", b"fixture"),
                                       ("upstream.bundle", b"invalid fixture bundle")):
                    entry = tarfile.TarInfo(name)
                    entry.size = len(contents)
                    archive.addfile(entry, io.BytesIO(contents))
            digest = hashlib.sha256(archive_path.read_bytes()).hexdigest()

            def local_remote(record, script):
                return subprocess.run(["bash", "-se"], input=script, check=True,
                                      text=True, capture_output=True)

            with patch.object(runner, "invoke"), patch.object(runner, "remote", local_remote):
                with self.assertRaises(subprocess.CalledProcessError) as result:
                    runner.execute_check({"ip": "fixture", "ssh_argv": ["ssh", "fixture"]},
                                         "linux", "dev", archive_path, digest, str(root))
            self.assertEqual(result.exception.returncode, 128)
            log = (root / "check.log").read_text()
            self.assertIn("Could not read from remote repository", log)
            self.assertTrue(result.exception.stdout.endswith(log))
            self.assertEqual(result.exception.stdout.count("Could not read from remote repository"), 1)
            self.assertFalse((root / "build").exists())

    def test_local_whitespace_error_stops_before_transfer(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            git(root, "init", "-q")
            (root / "source").write_text("new trailing whitespace \n")
            git(root, "add", ".")
            destination = root / "source.tar.gz"
            with patch.object(runner, "ROOT", root):
                with self.assertRaises(subprocess.CalledProcessError):
                    runner.snapshot(destination, root / "missing-upstream")
            self.assertFalse(destination.exists())

    def test_windows_large_script_uses_stdin(self):
        script = "Write-Output 'Unicode Юникод'\n" * 1000
        with patch.object(runner, "invoke") as invoke:
            runner.remote({"name": "runner-windows", "ssh_argv": ["ssh", "fixture"]}, script)
        args, kwargs = invoke.call_args
        self.assertLess(len(args[0][-1]), 2000)
        self.assertEqual(kwargs["input"], script)

    def test_snapshot_current_files_and_pinned_readonly_upstream(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / "project"
            upstream = Path(temporary) / "upstream"
            root.mkdir()
            upstream.mkdir()
            for directory in (root, upstream):
                git(directory, "init", "-q")
                git(directory, "config", "user.name", "Fixture")
                git(directory, "config", "user.email", "fixture@example.invalid")
            (upstream / "source").write_text("pinned")
            git(upstream, "add", ".")
            git(upstream, "commit", "-qm", "baseline")
            revision = git(upstream, "rev-parse", "HEAD")
            (upstream / "source").write_text("later")
            git(upstream, "commit", "-qam", "later")
            (upstream / "source").write_text("uncommitted")
            refs = git(upstream, "show-ref")
            status = git(upstream, "status", "--porcelain")
            (root / "docs").mkdir()
            (root / "docs/upstream-baseline.json").write_text(json.dumps({"revision": revision}))
            (root / "resources").mkdir()
            name = "resources/Юникод space.txt"
            (root / name).write_text("original")
            (root / "resources/deleted").write_text("deleted")
            git(root, "add", ".")
            (root / name).write_text("current")
            (root / "resources/deleted").unlink()
            (root / "resources/untracked-secret").write_text("excluded")
            (root / "build").mkdir()
            (root / "build/ignored-secret").write_text("excluded")
            destination = Path(temporary) / "source.tar.gz"
            with patch.object(runner, "ROOT", root):
                digest = runner.snapshot(destination, upstream)
            self.assertEqual(digest, hashlib.sha256(destination.read_bytes()).hexdigest())
            with tarfile.open(destination) as archive:
                self.assertEqual(archive.extractfile(name).read(), b"current")
                self.assertEqual(set(archive.getnames()),
                                 {name, "docs/upstream-baseline.json", "upstream.bundle"})
            clone = Path(temporary) / "clone"
            git(Path(temporary), "clone", "-q", "-b", "runner-baseline",
                str(Path(temporary) / "upstream.bundle"), str(clone))
            self.assertEqual(git(clone, "rev-parse", "HEAD"), revision)
            self.assertEqual((clone / "source").read_text(), "pinned")
            self.assertEqual(git(upstream, "show-ref"), refs)
            self.assertEqual(git(upstream, "status", "--porcelain"), status)

    def test_transfer_failure_and_cancellation_are_recorded(self):
        for error, expected in ((subprocess.CalledProcessError(17, "scp"), "failed"),
                                (KeyboardInterrupt(), "interrupted")):
            with self.subTest(status=expected), tempfile.TemporaryDirectory() as temporary:
                with patch.object(runner, "STATE", Path(temporary)), \
                        patch.object(runner, "snapshot", return_value="fixture-sha"), \
                        patch.object(runner, "execute_check", side_effect=error):
                    with self.assertRaises(type(error)):
                        runner.check({"name": "runner-linux", "ip": "fixture"},
                                     "linux", "dev", Path(temporary))
                records = list(Path(temporary).glob("*/run.json"))
                self.assertEqual(len(records), 1)
                record = json.loads(records[0].read_text())
                self.assertEqual(record["status"], expected)
                if expected == "failed":
                    self.assertEqual(record["exit_code"], 17)


if __name__ == "__main__":
    unittest.main()
