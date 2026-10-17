"""Offline source reconciliation, locking, guest context and failure regressions."""

import contextlib
import hashlib
import importlib.util
import io
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
sys.path.insert(0, str(ROOT / "scripts"))
spec = importlib.util.spec_from_file_location("hyperv_runner", ROOT / "scripts/hyperv-runner.py")
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
import hyperv_lock
import hyperv_source
import hyperv_transport
import hyperv_workspace


def git(root, *args):
    return subprocess.check_output(["git", "-C", str(root), *args], text=True).strip()


class RunnerTests(unittest.TestCase):
    def source_fixture(self, root):
        project, upstream = root / "project", root / "upstream"
        for path in (project, upstream):
            path.mkdir()
            git(path, "init", "-q")
            git(path, "config", "user.name", "Fixture")
            git(path, "config", "user.email", "fixture@example.invalid")
        (upstream / "source").write_text("pinned")
        git(upstream, "add", ".")
        git(upstream, "commit", "-qm", "baseline")
        revision = git(upstream, "rev-parse", "HEAD")
        (project / "docs").mkdir()
        (project / "docs/upstream-baseline.json").write_text(json.dumps({"revision": revision}))
        (project / "src").mkdir()
        (project / "src/changed.cpp").write_text("old")
        (project / "src/deleted.cpp").write_text("old")
        (project / ".gitignore").write_text("build/\nsrc/ignored-secret\n__pycache__/\n")
        git(project, "add", ".")
        git(project, "commit", "-qm", "baseline")
        return project, upstream

    def test_source_includes_current_new_unicode_files_and_deletions_without_private_state(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            project, upstream = self.source_fixture(root)
            (project / "src/changed.cpp").write_text("new")
            (project / "src/deleted.cpp").unlink()
            (project / "src/Юникод $(touch injected).cpp").write_text("new file")
            (project / "src/ignored-secret").write_text("excluded")
            (project / "build").mkdir()
            (project / "build/secret").write_text("excluded")
            (project / ".codex").mkdir()
            (project / ".codex/private").write_text("excluded")
            (upstream / "source").write_text("uncommitted; do not change")
            before = git(upstream, "status", "--porcelain")
            destination = root / "stage"
            destination.mkdir()
            digest = hyperv_source.stage_tree(project, upstream, destination, root / "cache")
            hyperv_source.verify_tree(destination, digest)
            self.assertEqual((destination / "src/changed.cpp").read_text(), "new")
            self.assertTrue((destination / "src/Юникод $(touch injected).cpp").exists())
            self.assertFalse((destination / "src/deleted.cpp").exists())
            self.assertFalse((destination / "src/ignored-secret").exists())
            self.assertFalse((destination / "build").exists())
            self.assertFalse((destination / ".codex").exists())
            self.assertEqual(before, git(upstream, "status", "--porcelain"))
            (destination / "src/changed.cpp").write_text("tampered")
            with self.assertRaises(ValueError):
                hyperv_source.verify_tree(destination, digest)

    def test_source_rejects_links_outside_project(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            project, upstream = self.source_fixture(root)
            try:
                (project / "src/link").symlink_to("../../upstream/source")
            except OSError as error:
                self.skipTest(f"Native symlink unavailable: {error}")
            destination = root / "stage"
            destination.mkdir()
            with self.assertRaises(ValueError):
                hyperv_source.stage_tree(project, upstream, destination, root / "cache")

    def test_rsync_deletes_stale_source_preserves_outputs_and_checks_same_mtime_content(self):
        with tempfile.TemporaryDirectory(prefix="sync Юникод ") as temporary:
            root = Path(temporary)
            source, destination = root / "source", root / "current"
            source.mkdir()
            destination.mkdir()
            (source / "src").mkdir()
            (destination / "src").mkdir()
            for path in (source / "src/current", destination / "src/current"):
                path.write_text("old" if destination in path.parents else "new")
                os.utime(path, (1000000000, 1000000000))
            (destination / "src/deleted").write_text("stale")
            for name in ("build/object.o", ".git/index", "runner-tmp/active", "upstream/config"):
                path = destination / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text("preserve")
            # Reproduce a source edit made before an earlier snapshot finished
            # building. Synchronizing its old mtime must not hide changed code.
            build = destination / "build"
            (build / "render.py").write_text(
                "from pathlib import Path\n"
                "Path('result').write_text(Path('../src/current').read_text())\n"
                "with Path('runs').open('a') as log: log.write('built\\n')\n")
            python = Path(sys.executable).as_posix().replace("$", "$$")
            (build / "build.ninja").write_text(
                f'rule render\n  command = "{python}" render.py\n'
                'build result: render ../src/current\n')
            def build_result():
                subprocess.run(["ninja", "-C", str(build)], check=True, capture_output=True)
                return (build / "result").read_text()
            self.assertEqual(build_result(), "old")
            record = {"ssh_argv": ["ssh", "fixture"], "ip": "fixture"}
            command = hyperv_transport.sync_command(record, "linux", source, str(destination))
            # Run the production reconciliation flags locally, without SSH.
            index = command.index("-e")
            command[index:index + 2] = []
            command = [arg for arg in command if not arg.startswith("--rsync-path=")]
            command[-1] = str(destination) + "/"
            if os.name == "nt":
                command[0] = hyperv_transport.WINDOWS_RSYNC
                for index in (-2, -1):
                    path = Path(command[index]).as_posix()
                    command[index] = "/" + path[0].lower() + path[2:] + "/"
            result = subprocess.run(command, check=True, capture_output=True)
            self.assertEqual((destination / "src/current").read_text(), "new", repr((command, result.stdout, result.stderr)))
            self.assertFalse((destination / "src/deleted").exists())
            self.assertEqual(build_result(), "new")
            for name in ("build/object.o", ".git/index", "runner-tmp/active", "upstream/config"):
                self.assertEqual((destination / name).read_text(), "preserve")
            second = subprocess.run(command, check=True, capture_output=True)
            self.assertRegex(second.stdout.decode("utf-8", errors="replace"),
                             r"Number of (?:regular )?files transferred: 0")
            runs = (build / "runs").read_bytes()
            self.assertEqual(build_result(), "new")
            self.assertEqual((build / "runs").read_bytes(), runs)

    def test_lock_serializes_sync_and_keeps_unconfirmed_or_wrong_owner(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / "current"
            hyperv_lock.operate(root, "first", "acquire")
            with self.assertRaises(ValueError):
                hyperv_lock.operate(root, "second", "acquire")
            with self.assertRaises(ValueError):
                hyperv_lock.operate(root, "second", "release")
            with self.assertRaises(ValueError):
                hyperv_lock.operate(root, "first", "release-completed")
            (root / ".runner-complete").write_text("first")
            hyperv_lock.operate(root, "first", "release-completed")
            hyperv_lock.operate(root, "second", "acquire")
            self.assertFalse((root / ".runner-complete").exists())
            hyperv_lock.operate(root, "second", "release")

    def test_unowned_existing_directory_is_never_adopted_or_deleted(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / "current"
            root.mkdir()
            (root / "private").write_text("preserve")
            with self.assertRaises(ValueError):
                hyperv_lock.operate(root, "first", "acquire")
            self.assertEqual((root / "private").read_text(), "preserve")
            self.assertFalse(root.with_name("current.lock").exists())

    def test_workspace_configuration_and_missing_mount_fail_before_sync(self):
        with tempfile.TemporaryDirectory() as temporary:
            state = Path(temporary)
            valid = {"linux": {"root": hyperv_workspace.LINUX_DATA_ROOT,
                               "filesystem_uuid": "9fd6574a-fe35-4af5-82fc-f1b16d78afb8"},
                     "windows": {"root": hyperv_workspace.DEFAULT_ROOTS["windows"]}}
            config = state / "workspaces.json"
            config.write_text(json.dumps(valid))
            self.assertEqual(hyperv_workspace.load_workspaces(state), valid)
            record = {"name": "runner-linux", "ssh_argv": ["ssh", "fixture"], "ip": "fixture"}
            with patch.object(runner, "discover", return_value=record), \
                    patch.object(runner, "remote", side_effect=subprocess.CalledProcessError(1, "mount")), \
                    patch.object(runner, "lock_command") as lock, patch.object(runner, "invoke") as invoke:
                result = runner.run_platform("linux", state, state, "hash", "dev", None, False, workspace=valid["linux"])
                self.assertEqual(result["directory"], valid["linux"]["root"] + "/current")
                self.assertEqual(result["status"], "failed")
                lock.assert_not_called()
                invoke.assert_not_called()
            for invalid in ("/unowned", "/mnt/sevenzip-desktop/runs/../other"):
                valid["linux"]["root"] = invalid
                config.write_text(json.dumps(valid))
                with self.assertRaises(ValueError):
                    hyperv_workspace.load_workspaces(state)

    def test_failed_command_keeps_exit_code_literal_args_context_and_completion(self):
        with tempfile.TemporaryDirectory(prefix="runner Юникод ") as temporary:
            root = Path(temporary)
            (root / "scripts").mkdir()
            shutil.copy2(ROOT / "scripts/hyperv_source.py", root / "scripts/hyperv_source.py")
            (root / "scripts/fixture.sh").write_text('printf "%s\\n" "$@" "$TEMP" "$QT_QPA_PLATFORM"\nexit 23\n')
            records = {name: {"sha256": hyperv_source.sha(root / name), "executable": bool((root / name).stat().st_mode & 0o111)}
                       for name in ("scripts/hyperv_source.py", "scripts/fixture.sh")}
            manifest = root / hyperv_source.MANIFEST
            manifest.write_text(json.dumps(records))
            args = ["space name", "Юникод", "$(touch injected)", "a'b", "", "first\nsecond"]
            def local_remote(record, script, **kwargs):
                return subprocess.run(["bash", "-se"], input=script, text=True, check=True, capture_output=True)
            with patch.object(runner, "remote", local_remote), \
                    patch.dict(os.environ, {"QT_QPA_PLATFORM": "offscreen"}):
                with self.assertRaises(subprocess.CalledProcessError) as error:
                    runner.execute_check({}, "linux", "dev", hyperv_source.sha(manifest), root.as_posix(), "token",
                                         ["bash", "scripts/fixture.sh", *args], False)
            self.assertEqual(error.exception.returncode, 23)
            log = (root / "check.log").read_text()
            self.assertIn("\n".join(args) + "\n", log)
            self.assertTrue(log.endswith("/runner-tmp\noffscreen\n"))
            self.assertEqual((root / ".runner-complete").read_text().strip(), "token")
            self.assertFalse((root / "injected").exists())
            self.assertEqual(git(root, "status", "--porcelain"), "")

    def test_custom_command_keeps_percent_variables_and_metacharacters_literal(self):
        payload = runner.command_payload("dev", ["printf", "%s\\n", "%PATH%", "$(touch injected)", "Юникод"])
        result = subprocess.run(["bash", "-se"], input=payload, text=True, capture_output=True, check=True)
        self.assertEqual(result.stdout, "%PATH%\n$(touch injected)\nЮникод\n")

    def test_release_payload_stops_after_failed_debug_gate(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "scripts").mkdir()
            (root / "scripts/check.sh").write_text("exit 29\n")
            (root / "scripts/build.sh").write_text("touch unexpected\n")
            result = subprocess.run(["bash", "-se"], input=runner.command_payload("release", None), cwd=root, text=True)
            self.assertEqual(result.returncode, 29)
            self.assertFalse((root / "unexpected").exists())

    def test_interrupted_execution_requests_confirmed_release_and_records_failure(self):
        with tempfile.TemporaryDirectory() as temporary:
            record = {"name": "runner-linux", "ssh_argv": ["ssh", "fixture"], "ip": "fixture"}
            current = Path(temporary) / "worktree/current"
            def local_lock(record, root, token, action, **kwargs):
                return hyperv_lock.operate(current, token, action)
            with patch.object(runner, "discover", return_value=record), \
                    patch.object(runner, "invoke"), patch.object(runner, "lock_command", side_effect=local_lock), \
                    patch.object(runner, "execute_check", side_effect=KeyboardInterrupt()):
                with self.assertRaises(KeyboardInterrupt):
                    runner.run_platform("linux", Path(temporary), Path(temporary), "hash", "dev", None, False)
                self.assertTrue(current.with_name("current.lock").is_dir())
            result = json.loads((Path(temporary) / "linux/run.json").read_text())
            self.assertEqual(result["status"], "interrupted")

    def test_cli_rejects_wrong_desktop_transport_and_unsafe_script_paths(self):
        for argv in (["check", "linux", "--interactive"], ["check", "windows", "--elevated"],
                     ["check", "linux", "--transport", "scp"], ["run", "all"],
                     ["run", "linux", "--", "../other.sh"]):
            with self.subTest(argv=argv), contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit):
                    runner.parse_arguments(argv)
        self.assertEqual(runner.parse_arguments(["status", "all"]).platforms, ["linux", "windows"])


if __name__ == "__main__":
    unittest.main()
