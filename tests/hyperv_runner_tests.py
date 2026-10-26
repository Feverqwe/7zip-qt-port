"""Offline source reconciliation, locking, guest context and failure regressions."""

import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
import uuid
from unittest.mock import Mock, patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
spec = importlib.util.spec_from_file_location("hyperv_runner", ROOT / "scripts/hyperv-runner.py")
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
from lib import cp2_snapshot, hyperv_interactive, hyperv_lock, hyperv_source, hyperv_transport, hyperv_workspace
from lib.runner_config import load_config
from lib.runner_timing import summarize


def git(root, *args):
    return subprocess.check_output(["git", "-C", str(root), *args], text=True).strip()


class RunnerTests(unittest.TestCase):
    def test_cp2_snapshot_rebuilds_edits_made_during_previous_build_and_recovers_lost_cache(self):
        if not shutil.which("cmake") or not shutil.which("ninja"):
            self.skipTest("CMake/Ninja unavailable")
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            project, upstream = self.source_fixture(base)
            (project / "CMakeLists.txt").write_text(
                'cmake_minimum_required(VERSION 3.20)\nproject(Freshness NONE)\n'
                'add_custom_command(OUTPUT result.txt COMMAND "${CMAKE_COMMAND}" -E copy '
                '"${CMAKE_SOURCE_DIR}/src/changed.cpp" result.txt DEPENDS "${CMAKE_SOURCE_DIR}/src/changed.cpp")\n'
                'add_custom_target(copy ALL DEPENDS result.txt)\n')
            cache = base / "time-cache.json"
            first, guest = base / "first", base / "guest"
            first.mkdir()
            hyperv_source.stage_tree(project, upstream, first, base / "bundles", source_times=True)
            entries = cp2_snapshot.prepare(first, guest, cache, time.time_ns())
            cp2_snapshot.commit(cache, entries)
            # The real source changes after the old snapshot, before the old
            # build finishes. Its old restored mtime must not hide that edit.
            original = project / "src/changed.cpp"
            kept_time = original.stat().st_mtime_ns
            original.write_text("new")
            os.utime(original, ns=(kept_time, kept_time))
            def build(*args):
                subprocess.run(["cmake", *args], check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            build("-G", "Ninja", "-S", str(guest), "-B", str(guest / "build"))
            build("--build", str(guest / "build"))
            output = guest / "build/result.txt"
            self.assertEqual(output.read_text(), "old")
            second = base / "second"
            second.mkdir()
            hyperv_source.stage_tree(project, upstream, second, base / "bundles", source_times=True)
            def sync(name):
                staged = base / name
                entries = cp2_snapshot.prepare(second, staged, cache, time.time_ns())
                shutil.copytree(staged, guest, dirs_exist_ok=True)
                cp2_snapshot.commit(cache, entries)
                build("--build", str(guest / "build"))
                self.assertEqual(output.read_text(), "new")
            sync("new-version")
            before = output.stat().st_mtime_ns
            sync("unchanged-version")
            self.assertEqual(output.stat().st_mtime_ns, before)
            (guest / "src/changed.cpp").unlink()
            sync("externally-deleted")
            cache.unlink()
            sync("lost-cache")
            cache.write_text("damaged cache")
            sync("damaged-cache")
            self.assertEqual(original.stat().st_mtime_ns, kept_time)

    def test_snapshot_retries_a_source_edit_during_copy(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            project, upstream = self.source_fixture(base)
            staged = base / "snapshot"
            staged.mkdir()
            original = shutil.copy2
            changed = project / "src/changed.cpp"
            edited = False
            def racing_copy(source, target):
                nonlocal edited
                result = original(source, target)
                if Path(source) == changed and not edited:
                    changed.write_text("updated during snapshot")
                    edited = True
                return result
            with patch.object(hyperv_source.shutil, "copy2", side_effect=racing_copy):
                hyperv_source.stage_tree(project, upstream, staged, base / "cache", source_times=True)
            self.assertEqual((staged / "src/changed.cpp").read_text(), "updated during snapshot")

    def test_test_start_timing_uses_guest_clock_and_rejects_previous_run(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "timing.jsonl"
            events = [("old", "tests_start", 1), ("current", "configure_start", 1002),
                      ("current", "configure_end", 1005), ("current", "tests_start", 1008),
                      ("current", "tests_start", 1010)]
            path.write_text("".join(json.dumps({"token": token, "event": event, "monotonic": stamp}) + "\n"
                                    for token, event, stamp in events))
            result = summarize(path, "current", (15, 1000, 17), 10)
            self.assertEqual(result["to_tests_seconds"], 14)
            self.assertEqual(result["to_tests_bounds_seconds"], [13, 15])
            self.assertEqual(result["guest_seconds"]["configure"], 3)
            self.assertNotIn("to_tests_seconds", summarize(path, "missing", (15, 1000, 17), 10))

    def test_interactive_branch_worktree_and_unsafe_roots(self):
        workspace = load_config()["windows"]["workspace"]
        root = hyperv_workspace.worktree_root(workspace, "feature/Btrfs images 空")
        # Must accept the same isolated branch root used by the transport.
        hyperv_interactive.interactive_command(root)
        for unsafe in (root + "/child", root + "'", workspace["root"] + "/../elsewhere",
                       workspace["root"] + "-other/current"):
            with self.subTest(root=unsafe), self.assertRaises(ValueError):
                hyperv_interactive.interactive_command(unsafe)

    def setUp(self):
        self.rsync = shutil.which("rsync")
        if os.name == "nt" and not self.rsync:
            try:
                self.rsync = load_config()["windows"]["rsync"]
            except (OSError, ValueError, KeyError):
                pass
        temporary = tempfile.TemporaryDirectory(prefix="runner config ")
        self.addCleanup(temporary.cleanup)
        self.config_path = Path(temporary.name) / "runners.local.json"
        shutil.copy2(ROOT / "scripts/config/runners.example.json", self.config_path)
        environment = patch.dict(os.environ, {"SEVENZIP_RUNNER_CONFIG": str(self.config_path)})
        environment.start()
        self.addCleanup(environment.stop)

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
            (project / "translations").mkdir()
            catalog = project / "translations/fixture.ts"
            catalog.write_text("current translation input")
            tool_asset = project / "tools/cp2/fixture.json"
            tool_asset.parent.mkdir(parents=True)
            tool_asset.write_text('{"tool": "current resource"}')
            (project / "src/ignored-secret").write_text("excluded")
            (project / "build").mkdir()
            (project / "build/secret").write_text("excluded")
            (project / ".codex").mkdir()
            (project / ".codex/private").write_text("excluded")
            (project / "scripts/config").mkdir(parents=True)
            (project / "scripts/config/runners.local.json").write_text("private connection settings")
            (upstream / "source").write_text("uncommitted; do not change")
            before = git(upstream, "status", "--porcelain")
            destination = root / "stage"
            destination.mkdir()
            hyperv_source.stage_tree(project, upstream, destination, root / "cache")
            self.assertEqual((destination / "src/changed.cpp").read_text(), "new")
            self.assertEqual((destination / "translations/fixture.ts").read_text(), catalog.read_text())
            self.assertEqual((destination / "tools/cp2/fixture.json").read_text(), tool_asset.read_text())
            self.assertTrue((destination / "src/Юникод $(touch injected).cpp").exists())
            self.assertFalse((destination / "src/deleted.cpp").exists())
            self.assertFalse((destination / "src/ignored-secret").exists())
            self.assertFalse((destination / "build").exists())
            self.assertFalse((destination / ".codex").exists())
            self.assertFalse((destination / "scripts/config/runners.local.json").exists())
            self.assertEqual(before, git(upstream, "status", "--porcelain"))
            with patch.dict(os.environ, {"SEVENZIP_RUN_TOKEN": "fixture"}):
                context = hyperv_source.runner_context(destination)
            self.assertEqual(context["revision"], git(project, "rev-parse", "HEAD"))
            self.assertTrue(context["sourceState"]["modified"])

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
        if not self.rsync:
            self.skipTest("rsync is not installed or configured")
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
                command[0] = self.rsync
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

    def test_busy_remote_lock_reports_owner_without_traceback_or_changing_lock(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / "guest"
            hyperv_lock.operate(root, "active-run", "acquire")
            def local_remote(record, script, **kwargs):
                return subprocess.run(["bash", "-se"], input=script, text=True,
                                      capture_output=True, check=True)
            record = {"platform": "linux", "config": {"python": Path(sys.executable).as_posix()}}
            with patch.object(runner, "remote", local_remote), \
                    self.assertRaises(subprocess.CalledProcessError) as error:
                runner.lock_command(record, str(root), "new-run", "acquire")
            self.assertIn("active-run", error.exception.stderr)
            self.assertNotIn("Traceback", error.exception.stderr)
            owner = json.loads((root.with_name("guest.lock") / "owner.json").read_text())
            self.assertEqual(owner["token"], "active-run")

    def test_unowned_existing_directory_is_never_adopted_or_deleted(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / "current"
            root.mkdir()
            (root / "private").write_text("preserve")
            with self.assertRaises(ValueError):
                hyperv_lock.operate(root, "first", "acquire")
            self.assertEqual((root / "private").read_text(), "preserve")
            self.assertFalse(root.with_name("current.lock").exists())

    def test_branch_worktrees_isolate_concurrent_runs_and_reuse_builds_across_checkouts(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            project, _ = self.source_fixture(base)
            git(project, "checkout", "-qb", "feature/Юникод")
            workspace = {"root": str(base / "guest")}
            branch = hyperv_workspace.current_branch(project)
            root = Path(hyperv_workspace.worktree_root(workspace, branch))
            hyperv_lock.operate(root, "first", "acquire")
            (root / "build").mkdir()
            (root / "build/result").write_text("retained build")
            others = []
            # Include names that collide under slash replacement or Windows
            # case folding; they must still acquire independent locks.
            for name in ("feature/other", "feature-other", "Feature-other"):
                other = Path(hyperv_workspace.worktree_root(workspace, name))
                hyperv_lock.operate(other, name, "acquire")
                others.append((other, name))
            clone = base / "another-checkout"
            git(base, "clone", "-q", str(project), str(clone))
            git(clone, "checkout", "-q", branch)
            reused = Path(hyperv_workspace.worktree_root(workspace, hyperv_workspace.current_branch(clone)))
            with self.assertRaisesRegex(ValueError, "busy"):
                hyperv_lock.operate(reused, "duplicate", "acquire")
            hyperv_lock.operate(root, "first", "release")
            hyperv_lock.operate(reused, "next", "acquire")
            self.assertEqual((reused / "build/result").read_text(), "retained build")
            hyperv_lock.operate(reused, "next", "release")
            for other, token in others:
                hyperv_lock.operate(other, token, "release")
            git(project, "checkout", "-q", "--detach")
            with self.assertRaisesRegex(ValueError, "detached HEAD"):
                hyperv_workspace.current_branch(project)

    def test_nested_worktree_runner_uses_shared_harness_and_its_own_uncommitted_source(self):
        with tempfile.TemporaryDirectory(prefix="worktree harness ") as temporary:
            base = Path(temporary)
            project, upstream = self.source_fixture(base)
            upstream.rename(base / "7zip-upstream")
            shutil.copytree(ROOT / "scripts/lib", project / "scripts/lib", ignore=shutil.ignore_patterns("__pycache__"))
            shutil.copy2(ROOT / "scripts/hyperv-runner.py", project / "scripts/hyperv-runner.py")
            (project / "scripts/fixture.py").write_text(
                "from pathlib import Path\nPath('observed').write_text(Path('src/changed.cpp').read_text())\n")
            (project / ".gitignore").write_text((project / ".gitignore").read_text() + "/.worktrees/\n/scripts/config/*.local.json\n")
            git(project, "add", ".")
            git(project, "commit", "-qm", "runner harness")
            worktree = project / ".worktrees/feature"
            git(project, "worktree", "add", "-qb", "feature/nested", str(worktree))
            (worktree / "src/changed.cpp").write_text("feature edits")
            (project / "src/changed.cpp").write_text("main edits must not travel")
            settings = json.loads(self.config_path.read_text())
            settings["runners"] = {"linux": settings["runners"]["linux"]}
            settings["runners"]["linux"].update(ssh=["ssh", "fixture"], python=sys.executable,
                                                workspace={"root": "/fixture/sevenzip/runs"})
            (project / "scripts/config").mkdir()
            (project / "scripts/config/runners.local.json").write_text(json.dumps(settings))
            # Replace only guest transport/execution with a local guest. Keep
            # CLI selection, config discovery, staging, locks and reports real.
            driver = base / "drive.py"
            driver.write_text('''import importlib.util
from pathlib import Path
import shutil
import subprocess
import sys
sys.path.insert(0, str(Path.cwd() / 'scripts'))
from lib import hyperv_lock
spec = importlib.util.spec_from_file_location('runner', 'scripts/hyperv-runner.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
def guest_path(root):
    return Path(__file__).parent / 'guest' / root.rsplit('/', 1)[-1]
def sync(argv, **kwargs):
    shutil.copytree(argv[-2], guest_path(argv[-1].split(':', 1)[1].rstrip('/')), dirs_exist_ok=True)
def lock(record, root, token, action, **kwargs):
    hyperv_lock.operate(guest_path(root), token, action)
def execute(record, platform, preset, root, token, command, *args):
    subprocess.run(command, cwd=guest_path(root), check=True)
    (guest_path(root) / '.runner-complete').write_text(token)
runner.invoke = sync
runner.lock_command = lock
runner.execute_check = execute
runner.collect_logs = lambda *args: {}
sys.argv = ['runner', 'run', 'linux', '--transport', 'rsync', '--skip-runtime', '--', 'scripts/fixture.py']
raise SystemExit(runner.main())
''')
            environment = dict(os.environ)
            environment.pop("SEVENZIP_RUNNER_CONFIG", None)
            result = subprocess.run([sys.executable, str(driver)], cwd=worktree, env=environment,
                                    text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            reports = list((project / "build/runners/hyperv").glob("*/run.json"))
            self.assertEqual(len(reports), 1)
            report = json.loads(reports[0].read_text())
            self.assertEqual(report["branch"], "feature/nested")
            guest = base / "guest" / report["platforms"][0]["directory"].rsplit("/", 1)[-1]
            self.assertEqual((guest / "observed").read_text(), "feature edits")
            self.assertFalse(guest.with_name(guest.name + ".lock").exists())
            self.assertFalse((guest / "scripts/config/runners.local.json").exists())
            self.assertFalse((worktree / "build/runners").exists())

    def test_workspace_configuration_and_missing_mount_fail_before_sync(self):
        with tempfile.TemporaryDirectory() as temporary:
            state = Path(temporary)
            valid = {"linux": {"root": "/mnt/custom-data/sevenzip/runs", "mount": "/mnt/custom-data",
                               "filesystem_uuid": str(uuid.uuid4())},
                     "windows": {"root": "D:/build/sevenzip/runs"}}
            settings = {"schema": 1, "runners": load_config()}
            for platform in valid:
                settings["runners"][platform]["workspace"] = valid[platform]
            self.config_path.write_text(json.dumps(settings))
            self.assertEqual(hyperv_workspace.load_workspaces(), valid)
            record = {"name": "runner-linux", "ssh_argv": ["ssh", "fixture"], "ip": "fixture"}
            with patch.object(runner, "discover", return_value=record), \
                    patch.object(runner, "remote", side_effect=subprocess.CalledProcessError(1, "mount")), \
                    patch.object(runner, "lock_command") as lock, patch.object(runner, "invoke") as invoke:
                result = runner.run_platform("linux", state, state, "dev", None, False,
                                             workspace=valid["linux"], branch="fixture")
                self.assertEqual(result["directory"], hyperv_workspace.worktree_root(
                    valid["linux"], "fixture"))
                self.assertEqual(result["status"], "failed")
                lock.assert_not_called()
                invoke.assert_not_called()
            for invalid in ("/unowned", "/mnt/custom-data/runs/../other"):
                valid["linux"]["root"] = invalid
                settings["runners"]["linux"]["workspace"] = valid["linux"]
                self.config_path.write_text(json.dumps(settings))
                with self.assertRaises(ValueError):
                    hyperv_workspace.load_workspaces()

    def test_missing_config_fails_before_connecting(self):
        self.config_path.unlink()
        with patch.object(runner, "invoke") as invoke:
            with self.assertRaisesRegex(ValueError, "configuration missing"):
                runner.discover("linux")
            invoke.assert_not_called()

    def test_missing_timing_still_collects_verification_and_command_log(self):
        with tempfile.TemporaryDirectory() as temporary:
            destination = Path(temporary)
            def download(command, **kwargs):
                path = Path(command[-1])
                if path.name == "timing.jsonl":
                    path.write_text("partial transfer")
                    raise subprocess.CalledProcessError(1, command)
                if path.name == "check.log":
                    path.write_text("Command diagnostics retained")
                else:
                    path.write_text(json.dumps({"status": "passed", "run_token": "current"}))
            result = hyperv_transport.collect_logs(
                {"ssh_argv": ["ssh", "fixture"]}, "/guest", destination,
                ["python", "scripts/verify.py"], "current", download, None)
            self.assertEqual(result["verification"]["status"], "passed")
            self.assertIn("timing_error", result)
            self.assertEqual((destination / "check.log").read_text(), "Command diagnostics retained")
            self.assertFalse((destination / "timing.jsonl").exists())

    def test_connections_use_configured_alias_port_and_nondefault_windows_drive(self):
        settings = {"schema": 1, "runners": load_config()}
        settings["runners"]["windows"]["ssh"] = ["ssh", "-p", "2222", "builder@custom-host"]
        self.config_path.write_text(json.dumps(settings))
        with patch.object(runner, "STATE", self.config_path.parent / "state"):
            record = runner.discover("windows")
        record["name"] = "custom-builder"
        command = hyperv_transport.sync_command(record, "windows", Path("source"), "D:/build/custom/runs/current")
        self.assertEqual(command[-1], "builder@custom-host:/d/build/custom/runs/current/")
        with patch.object(runner, "invoke") as invoke:
            runner.remote(record, "Write-Output configured")
            self.assertIn("powershell.exe", invoke.call_args.args[0][-1])
        invoke = Mock()
        hyperv_transport.collect_logs(record, "D:/build/custom/runs/current", Path("logs"),
                                      None, "token", invoke, None)
        command = invoke.call_args_list[0].args[0]
        self.assertEqual(command[:4], ["scp", "-P", "2222", "builder@custom-host:D:/build/custom/runs/current/check.log"])

    def test_custom_readiness_command_supplies_connection_without_fixed_vm_names(self):
        provider = self.config_path.parent / "discover.py"
        provider.write_text("print(" + repr(json.dumps({"vm": "my-build-vm", "vm_id": "fixture",
                            "ssh_argv": ["ssh", "my-dynamic-host"]})) + ")\n")
        settings = {"schema": 1, "runners": load_config()}
        config = settings["runners"]["linux"]
        config.pop("ssh")
        config["ready_command"] = [sys.executable, str(provider)]
        self.config_path.write_text(json.dumps(settings))
        with patch.object(runner, "STATE", self.config_path.parent / "state"):
            record = runner.discover("linux")
        command = hyperv_transport.sync_command(record, "linux", Path("source"), config["workspace"]["root"] + "/current")
        self.assertTrue(command[-1].startswith("my-dynamic-host:"))

    def test_windows_host_guard_uses_configuration_and_rejects_a_different_machine(self):
        expected = load_config()["windows"]["computer_name"]
        for host, succeeds in ((expected.lower(), True), ("other-host", False)):
            environment = dict(os.environ, COMPUTERNAME=host, SEVENZIP_RUNNER_PYTHON=sys.executable)
            environment.pop("SEVENZIP_WINDOWS_RUNNER_HOST", None)
            result = subprocess.run(["bash", "-c", "source scripts/lib/windows-runner.sh; sevenzip_require_windows_runner"],
                                    cwd=ROOT, env=environment, text=True, capture_output=True)
            self.assertEqual(result.returncode == 0, succeeds, result.stderr)

    def test_failed_command_keeps_exit_code_literal_args_context_and_completion(self):
        with tempfile.TemporaryDirectory(prefix="runner Юникод ") as temporary:
            root = Path(temporary)
            (root / "scripts").mkdir()
            (root / "scripts/lib").mkdir()
            shutil.copy2(ROOT / "scripts/lib/runner_timing.py", root / "scripts/lib/runner_timing.py")
            (root / "scripts/fixture.sh").write_text('printf "%s\\n" "$@" "$TEMP" "$QT_QPA_PLATFORM" "$CMAKE_BUILD_PARALLEL_LEVEL"\nexit 23\n')
            args = ["space name", "Юникод", "$(touch injected)", "a'b", "", "first\nsecond"]
            def local_remote(record, script, **kwargs):
                return subprocess.run(["bash", "-se"], input=script, text=True, check=True, capture_output=True)
            with patch.object(runner, "remote", local_remote), \
                    patch.dict(os.environ, {"QT_QPA_PLATFORM": "offscreen", "CMAKE_BUILD_PARALLEL_LEVEL": "1"}):
                with self.assertRaises(subprocess.CalledProcessError) as error:
                    runner.execute_check({}, "linux", "dev", root.as_posix(), "token",
                                         ["bash", "scripts/fixture.sh", *args], False)
            self.assertEqual(error.exception.returncode, 23)
            log = (root / "check.log").read_text()
            self.assertIn("\n".join(args) + "\n", log)
            self.assertTrue(log.endswith("/runner-tmp\noffscreen\n1\n"))
            self.assertEqual((root / ".runner-complete").read_text().strip(), "token")
            self.assertFalse((root / "injected").exists())
            self.assertFalse((root / ".git").exists())

    def test_invalid_build_parallelism_is_rejected_before_remote_execution(self):
        for platform in ("linux", "windows"):
            for value in ("0", "-1", "", "1; touch injected"):
                with self.subTest(platform=platform, value=value), \
                        patch.dict(os.environ, {"CMAKE_BUILD_PARALLEL_LEVEL": value}), \
                        patch.object(runner, "remote") as remote:
                    with self.assertRaises(ValueError):
                        runner.execute_check({}, platform, "dev", "/unused", "token", prepare_runtime=False)
                    remote.assert_not_called()

    def test_gate_accepts_runner_snapshot_without_project_git_and_keeps_local_diff_check(self):
        with tempfile.TemporaryDirectory(prefix="gate snapshot ") as temporary:
            root = Path(temporary)
            (root / "scripts/lib").mkdir(parents=True)
            (root / "bin").mkdir()
            shutil.copy2(ROOT / "scripts/check.sh", root / "scripts/check.sh")
            shutil.copy2(ROOT / "scripts/lib/agent-output.sh", root / "scripts/lib/agent-output.sh")
            for name, content in (("bin/git", "exit 73\n"), ("bin/clang-format", "exit 0\n"),
                                  ("scripts/build.sh", "touch built\n"), ("scripts/ctest.sh", "exit 0\n")):
                path = root / name
                path.write_text("#!/usr/bin/env bash\n" + content)
                path.chmod(0o755)
            environment = dict(os.environ, PATH=str(root / "bin") + os.pathsep + os.environ["PATH"],
                               CODEX_SANDBOX="none", SEVENZIP_RUN_TOKEN="")
            local = subprocess.run(["bash", "scripts/check.sh"], cwd=root, env=environment,
                                   text=True, capture_output=True)
            self.assertEqual(local.returncode, 73, local.stdout + local.stderr)
            self.assertFalse((root / "built").exists())
            environment["SEVENZIP_RUN_TOKEN"] = "fixture"
            guest = subprocess.run(["bash", "scripts/check.sh"], cwd=root, env=environment,
                                   text=True, capture_output=True)
            self.assertEqual(guest.returncode, 0, guest.stdout + guest.stderr)
            self.assertTrue((root / "built").exists())

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

    def test_release_payload_builds_after_gate_without_testing_disabled_release(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "scripts").mkdir()
            (root / "scripts/check.sh").write_text("touch gate-passed\n")
            (root / "scripts/build.sh").write_text(
                'test -f gate-passed || exit 31\n'
                'test "$1" = release || exit 32\n'
                'touch release-built\n')
            # Release has BUILD_TESTING=OFF: asking CTest for it is an error.
            (root / "scripts/ctest.sh").write_text("exit 33\n")
            result = subprocess.run(["bash", "-se"], input=runner.command_payload("release", None),
                                    cwd=root, text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertTrue((root / "release-built").exists())

    def test_interrupted_execution_requests_confirmed_release_and_records_failure(self):
        with tempfile.TemporaryDirectory() as temporary:
            record = {"name": "runner-linux", "ssh_argv": ["ssh", "fixture"], "ip": "fixture"}
            current = Path(temporary) / "worktree/current"
            def local_lock(record, root, token, action, **kwargs):
                return hyperv_lock.operate(current, token, action)
            with patch.object(runner, "discover", return_value=record), \
                    patch.object(runner, "invoke"), patch.object(runner, "lock_command", side_effect=local_lock), \
                    patch.object(runner.hyperv_cp2_sync, "synchronize"), \
                    patch.object(runner, "execute_check", side_effect=KeyboardInterrupt()):
                with self.assertRaises(KeyboardInterrupt):
                    runner.run_platform("linux", Path(temporary), Path(temporary), "dev", None, False,
                                        branch="fixture")
                self.assertTrue(current.with_name("current.lock").is_dir())
            result = json.loads((Path(temporary) / "linux/run.json").read_text())
            self.assertEqual(result["status"], "interrupted")

    @unittest.skipUnless(os.name == "nt", "Windows PowerShell script dispatch")
    def test_powershell_script_dispatch_works_with_restricted_process_default(self):
        with tempfile.TemporaryDirectory(prefix="PowerShell script ") as temporary:
            root = Path(temporary)
            git(root, "init", "-q")
            (root / "scripts").mkdir()
            (root / "scripts/fixture.ps1").write_text("Write-Output $args[0]\n")
            with patch.object(runner, "ROOT", root):
                command = runner.script_command(["scripts/fixture.ps1", "literal space $(not-executed)"],
                                                ["windows"])
            environment = dict(os.environ, PSExecutionPolicyPreference="Restricted")
            result = subprocess.run(command, cwd=root, env=environment, text=True,
                                    capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual(result.stdout.strip(), "literal space $(not-executed)")

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
