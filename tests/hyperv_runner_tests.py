"""Offline checks for runner snapshots and failure records; no VM access."""

import hashlib
import base64
import importlib.util
import json
import io
import os
import contextlib
import re
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
spec = importlib.util.spec_from_file_location(
    "hyperv_runner", Path(__file__).resolve().parents[1] / "scripts/hyperv-runner.py")
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
import hyperv_transport
import hyperv_workspace


def git(directory, *args):
    return subprocess.check_output(["git", "-C", str(directory), *args], text=True).strip()


class RunnerTests(unittest.TestCase):
    @staticmethod
    def data_workspaces():
        return {"linux": {"root": "/mnt/sevenzip-desktop/runs",
                          "filesystem_uuid": "9fd6574a-fe35-4af5-82fc-f1b16d78afb8"},
                "windows": {"root": "C:/runner/sevenzip-desktop/runs"}}

    def test_workspace_configuration_validates_boundaries(self):
        with tempfile.TemporaryDirectory() as temporary:
            state = Path(temporary)
            self.assertEqual(hyperv_workspace.load_workspaces(state),
                             {p: {"root": root} for p, root in hyperv_workspace.DEFAULT_ROOTS.items()})
            path = state / "workspaces.json"
            valid = self.data_workspaces()
            path.write_text(json.dumps(valid))
            self.assertEqual(hyperv_workspace.load_workspaces(state), valid)
            invalid = [[], {}, {"linux": valid["linux"]},
                       {**valid, "windows": {"root": "D:/other-project"}},
                       {**valid, "linux": {"root": "/mnt/sevenzip-desktop/runs/../other"}},
                       {**valid, "linux": {"root": valid["linux"]["root"]}},
                       {**valid, "linux": {**valid["linux"], "filesystem_uuid": "$(id)"}},
                       {**valid, "windows": {**valid["windows"], "filesystem_uuid": "unused"}}]
            for value in invalid:
                with self.subTest(value=value):
                    path.write_text(json.dumps(value))
                    with self.assertRaises(ValueError):
                        hyperv_workspace.load_workspaces(state)

    def test_missing_data_mount_prevents_source_sync(self):
        with tempfile.TemporaryDirectory() as temporary:
            state = Path(temporary)
            (state / "workspaces.json").write_text(json.dumps(self.data_workspaces()))
            local = state / "20261007T000000Z-1234abcd"
            local.mkdir()
            record = {"name": "runner-linux", "ip": "fixture", "ssh_argv": ["ssh", "fixture"]}
            with patch.object(runner, "STATE", state), \
                    patch.object(runner, "discover", return_value=record), \
                    patch.object(runner, "remote", side_effect=subprocess.CalledProcessError(1, "mount guard")), \
                    patch.object(runner, "execute_check") as execute, patch.object(runner, "invoke"):
                result = runner.run_platform("linux", local, Path("fixture"), "hash", "dev", None, True)
            self.assertEqual(result["status"], "failed")
            self.assertEqual(result["exit_code"], 1)
            self.assertEqual(result["directory"], self.data_workspaces()["linux"]["root"] + "/" + local.name)
            execute.assert_not_called()

    def test_all_routes_workspaces_and_checks_only_the_data_mount(self):
        with tempfile.TemporaryDirectory() as temporary:
            state = Path(temporary)
            configured = self.data_workspaces()
            (state / "workspaces.json").write_text(json.dumps(configured))
            def discover(platform):
                return {"name": "runner-" + platform, "ip": "fixture", "ssh_argv": ["ssh", "fixture"]}
            with patch.object(runner, "STATE", state), \
                    patch.object(runner, "snapshot", return_value="same-sha"), \
                    patch.object(runner, "discover", side_effect=discover), \
                    patch.object(runner, "remote") as remote, \
                    patch.object(runner, "execute_check") as execute, patch.object(runner, "invoke"):
                self.assertEqual(runner.run_platforms(["linux", "windows"], "dev", state), 0)
            self.assertEqual(execute.call_count, 2)
            for call in execute.call_args_list:
                self.assertTrue(call.args[5].startswith(configured[call.args[1]]["root"] + "/"))
            remote.assert_called_once()
            self.assertEqual(remote.call_args.args[0]["name"], "runner-linux")
            self.assertIn(configured["linux"]["filesystem_uuid"], remote.call_args.args[1])

    def test_rsync_basis_stays_on_selected_workspace(self):
        with tempfile.TemporaryDirectory() as temporary:
            state = Path(temporary)
            run_id = "20261007T000000Z-1234abcd"
            directory = state / run_id / "linux"
            directory.mkdir(parents=True)
            record = {"directory": hyperv_workspace.DEFAULT_ROOTS["linux"] + "/" + run_id,
                      "status": "passed", "transport": "rsync"}
            path = directory / "run.json"
            path.write_text(json.dumps(record))
            root = self.data_workspaces()["linux"]["root"]
            self.assertIsNone(hyperv_transport.previous_snapshot(state, "linux", root))
            record["directory"] = root + "/" + run_id
            path.write_text(json.dumps(record))
            self.assertEqual(hyperv_transport.previous_snapshot(state, "linux", root), record["directory"])
            self.assertIsNone(hyperv_transport.previous_snapshot(state, "linux"))
            with self.assertRaises(ValueError):
                hyperv_transport.previous_snapshot(state, "linux", "/unowned")

    def test_action_help_and_options_are_scoped(self):
        self.assertEqual(runner.parse_arguments(["probe", "all"]).platforms,
                         ["linux", "windows"])
        args = runner.parse_arguments(["check", "all", "--preset", "release",
                                       "--skip-runtime", "--upstream", "/fixture"])
        self.assertEqual(args.preset, "release")
        self.assertTrue(args.skip_runtime)
        self.assertEqual(args.upstream, Path("/fixture"))
        self.assertTrue(runner.parse_arguments(["check", "windows", "--interactive"]).interactive)
        self.assertTrue(runner.parse_arguments(["check", "all", "--interactive"]).interactive)
        self.assertTrue(runner.parse_arguments(
            ["check", "windows", "--interactive", "--elevated"]).elevated)
        for argv in (["probe", "linux", "--transport", "scp"],
                     ["run", "linux", "--preset", "release"],
                     ["check", "linux", "--"], ["probe", "all", "--"],
                     ["run", "all"], ["check", "linux", "--interactive"],
                     ["probe", "windows", "--interactive"],
                     ["check", "windows", "--elevated"],
                     ["check", "linux", "--interactive", "--elevated"],
                     ["probe", "windows", "--elevated"]):
            with self.subTest(argv=argv), contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit) as error:
                    runner.parse_arguments(argv)
                self.assertEqual(error.exception.code, 2)
        output = io.StringIO()
        with contextlib.redirect_stdout(output), self.assertRaises(SystemExit) as error:
            runner.parse_arguments(["run", "--help"])
        self.assertEqual(error.exception.code, 0)
        self.assertIn("-- <project-script>", output.getvalue())
        self.assertNotIn("--preset", output.getvalue())

    def test_probe_all_reports_both_guests_when_one_fails(self):
        def probe(platform):
            if platform == "linux":
                raise subprocess.CalledProcessError(17, "fixture", output="partial probe\n",
                                                    stderr="identity failed\n")
            return subprocess.CompletedProcess("fixture", 0, "Windows tools\n", "")
        stdout, stderr = io.StringIO(), io.StringIO()
        with patch.object(runner, "probe_platform", side_effect=probe) as mock, \
                contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            self.assertEqual(runner.probe_platforms(["linux", "windows"]), 1)
        self.assertEqual(mock.call_count, 2)
        self.assertIn("[windows] passed\nWindows tools", stdout.getvalue())
        self.assertIn("[linux] failed", stderr.getvalue())
        self.assertIn("identity failed", stderr.getvalue())
        self.assertIn("partial probe", stderr.getvalue())

    def test_release_payload_stops_before_later_stages_on_both_platforms(self):
        for platform in ("linux", "windows"):
            with self.subTest(platform=platform), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                (root / "scripts").mkdir()
                (root / "scripts/check.sh").write_text("echo debug\nexit 29\n")
                (root / "scripts/build.sh").write_text("echo release > unexpected\n")
                (root / "scripts/ctest.sh").write_text("echo tests >> unexpected\n")
                with patch.object(runner, "invoke"), patch.object(runner, "remote") as remote:
                    runner.execute_check({"ip": "fixture", "ssh_argv": ["ssh", "fixture"]},
                                         platform, "release", Path("fixture"), "hash", str(root),
                                         prepare_runtime=False)
                script = remote.call_args.args[1]
                if platform == "windows":
                    encoded = re.search(r"runner-command.sh', \[Convert\]::FromBase64String\('([^']+)'", script)
                    payload = base64.b64decode(encoded.group(1)).decode()
                else:
                    payload = script.split("(\nset -e\n", 1)[1].split(") > check.log", 1)[0]
                self.assertIn("bash scripts/build.sh release", payload)
                self.assertIn("bash scripts/ctest.sh --preset release", payload)
                result = subprocess.run(["bash", "-se"], input=payload, cwd=root,
                                        text=True, capture_output=True)
                self.assertEqual(result.returncode, 29)
                self.assertEqual(result.stdout, "debug\n")
                self.assertFalse((root / "unexpected").exists())

    def test_discovery_stderr_is_kept_in_failure_record_and_log(self):
        with tempfile.TemporaryDirectory() as temporary:
            error = subprocess.CalledProcessError(17, "discovery", stderr="guest identity failed\n")
            with patch.object(runner, "discover", side_effect=error):
                result = runner.run_platform("linux", Path(temporary), Path("fixture"),
                                             "hash", "dev", None, True)
            self.assertIn("guest identity failed", result["error"])
            self.assertEqual((Path(temporary) / "linux/transport.log").read_text(), error.stderr)

    def test_previous_rsync_basis_is_platform_specific_and_validated(self):
        with tempfile.TemporaryDirectory() as temporary:
            state = Path(temporary)
            run_id = "20261003T120000Z-1234abcd"
            directory = state / run_id / "linux"
            directory.mkdir(parents=True)
            record = {"directory": "/home/runner/sevenzip-desktop/runs/" + run_id,
                      "status": "passed", "transport": "rsync"}
            path = directory / "run.json"
            path.write_text(json.dumps(record))
            self.assertEqual(hyperv_transport.previous_snapshot(state, "linux"), record["directory"])
            self.assertIsNone(hyperv_transport.previous_snapshot(state, "windows"))
            record["directory"] += "/../../unrelated"
            path.write_text(json.dumps(record))
            self.assertIsNone(hyperv_transport.previous_snapshot(state, "linux"))

    def test_rsync_reuses_private_basis_and_transfers_changed_blocks(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            previous = root / "previous"
            destination = root / "destination"
            previous.mkdir()
            destination.mkdir()
            old = bytes(range(256)) * 1024
            new = old[:100000] + b"changed bytes" + old[100013:]
            (previous / "source.tar").write_bytes(old)
            archive = root / "source.tar"
            archive.write_bytes(new)
            stats = []
            def local_remote(record, script, **kwargs):
                subprocess.run(["bash", "-se"], input=script, text=True, check=True)
            def local_invoke(args, **kwargs):
                result = subprocess.run(["rsync", "--times", "--checksum", "--stats",
                                         "--no-whole-file", str(archive), str(destination / archive.name)],
                                        check=True, text=True, capture_output=True)
                stats.append(result.stdout)
            hyperv_transport.transfer({"ip": "fixture", "ssh_argv": ["ssh", "fixture"]},
                                      "linux", archive, str(destination), "rsync", str(previous),
                                      local_invoke, local_remote, {})
            self.assertEqual((destination / "source.tar").read_bytes(), new)
            self.assertEqual((previous / "source.tar").read_bytes(), old)
            matched = re.search(r"Matched data: ([0-9,]+) (?:bytes|B)", stats[0])
            self.assertIsNotNone(matched, stats[0])
            self.assertGreater(int(matched.group(1).replace(",", "")), 200000)

    def test_windows_rsync_uses_msys_path_and_verified_ssh_options(self):
        with patch.object(runner, "invoke") as invoke, patch.object(runner, "remote"):
            hyperv_transport.transfer(
                {"ip": "fixture", "ssh_argv": ["ssh", "-i", "/key with spaces", "runner@fixture"]},
                "windows", Path("source.tar"), "C:/runner/sevenzip-desktop/runs/new", "rsync", None,
                invoke, runner.remote, {})
        args = invoke.call_args.args[0]
        self.assertIn("--rsync-path=" + hyperv_transport.WINDOWS_RSYNC, args)
        self.assertEqual(args[-1], "runner@fixture:/c/runner/sevenzip-desktop/runs/new/source.tar")
        self.assertEqual(args[args.index("-e") + 1], "ssh -i '/key with spaces'")

    def test_cli_separator_and_selected_script_validation(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            git(root, "init", "-q")
            (root / "scripts").mkdir()
            (root / "scripts/fixture.sh").write_text("exit 0\n")
            with patch.object(runner, "ROOT", root):
                with self.assertRaisesRegex(ValueError, "git add"):
                    runner.script_command(["scripts/fixture.sh"], ["linux"])
                git(root, "add", "scripts/fixture.sh")
                argv = ["runner", "run", "all", "--skip-runtime", "--",
                        "scripts/fixture.sh", "--preset", "release", "space name"]
                with patch.object(runner.sys, "argv", argv), \
                        patch.object(runner, "run_platforms", return_value=0) as run:
                    self.assertEqual(runner.main(), 0)
                args = run.call_args.args
                self.assertEqual(args[0], ["linux", "windows"])
                self.assertEqual(args[3], ["bash", *argv[5:]])
                self.assertFalse(args[4])
                elevated_argv = argv[:4] + ["--interactive", "--elevated"] + argv[4:]
                with patch.object(runner.sys, "argv", elevated_argv), \
                        patch.object(runner, "run_platforms", return_value=0) as run:
                    self.assertEqual(runner.main(), 0)
                self.assertEqual(run.call_args.args[-2:], (True, True))
                with self.assertRaises(ValueError):
                    runner.script_command(["../outside.sh"], ["linux"])

    def test_all_uses_one_snapshot_and_keeps_other_platform_on_failure(self):
        with tempfile.TemporaryDirectory() as temporary:
            state = Path(temporary)
            def execute(record, platform, *args):
                if platform == "linux":
                    raise subprocess.CalledProcessError(17, "fixture")
            def discover(platform):
                return {"name": "runner-" + platform, "ip": "fixture",
                        "ssh_argv": ["ssh", "fixture"]}
            with patch.object(runner, "STATE", state), \
                    patch.object(runner, "snapshot", return_value="same-sha") as snapshot, \
                    patch.object(runner, "discover", side_effect=discover), \
                    patch.object(runner, "execute_check", side_effect=execute) as execute_mock, \
                    patch.object(runner, "invoke"):
                code = runner.run_platforms(["linux", "windows"], "dev", state,
                                            ["bash", "scripts/fixture.sh", "a b"],
                                            interactive=True, elevated=True)
            self.assertEqual(code, 1)
            snapshot.assert_called_once()
            self.assertEqual(execute_mock.call_count, 2)
            for call in execute_mock.call_args_list:
                self.assertEqual(call.args[-2:], (call.args[1] == "windows",) * 2)
            report = json.loads(next(state.glob("*/run.json")).read_text())
            self.assertEqual(report["status"], "failed")
            self.assertTrue(report["interactive"])
            self.assertTrue(report["elevated"])
            self.assertEqual([item["interactive"] for item in report["platforms"]], [False, True])
            self.assertEqual([item["elevated"] for item in report["platforms"]], [False, True])
            self.assertEqual([item["status"] for item in report["platforms"]], ["failed", "passed"])
            self.assertEqual({item["sha256"] for item in report["platforms"]}, {"same-sha"})
            self.assertEqual(report["platforms"][0]["exit_code"], 17)

    def test_interactive_elevation_checks_actual_token_before_running_gate(self):
        with self.assertRaises(ValueError):
            runner.run_platforms(["windows"], "dev", Path("unused"), elevated=True)
        with self.assertRaises(ValueError):
            runner.run_platforms(["linux"], "dev", Path("unused"),
                                 interactive=True, elevated=True)
        root = "C:/runner/sevenzip-desktop/runs/20261006T010000Z-1234abcd"
        for elevated, level in ((False, "Limited"), (True, "Highest")):
            with self.subTest(elevated=elevated):
                command = runner.interactive_command(root, elevated)
                self.assertIn("-RunLevel " + level, command)
                encoded = re.search(r"-EncodedCommand ([A-Za-z0-9+/=]+)", command).group(1)
                child = base64.b64decode(encoded).decode("utf-16le")
                self.assertIn("IsInRole", child)
                self.assertIn("RunLevel=" + level + " Elevated=$elevated", child)
                guard = "if (" + ("$true" if elevated else "$false") + " -and !$elevated)"
                self.assertIn(guard, child)
                self.assertLess(child.index(guard), child.index("& cmd.exe"))

    def test_interactive_tasks_request_normal_priority_and_verify_child_context(self):
        root = "C:/runner/sevenzip-desktop/runs/20261006T010000Z-1234abcd"
        for elevated in (False, True):
            with self.subTest(elevated=elevated):
                command = runner.interactive_command(root, elevated)
                settings = next(line for line in command.splitlines()
                                if line.startswith("$settings=New-ScheduledTaskSettingsSet "))
                priority = re.search(r"-Priority\s+([0-9]+)", settings)
                self.assertIsNotNone(priority, "Default7 is a background task, not an interactive run")
                self.assertIn(int(priority.group(1)), (4, 5, 6))
                encoded = re.search(r"-EncodedCommand ([A-Za-z0-9+/=]+)", command).group(1)
                child = base64.b64decode(encoded).decode("utf-16le")
                self.assertIn("Priority=$runnerPriority", child)
                self.assertIn("$env:SEVENZIP_RUNNER_EXPECT_NORMAL_PRIORITY='1'", child)
                guard = "if ($runnerPriority -ne 'Normal')"
                self.assertIn(guard, child)
                self.assertLess(child.index(guard), child.index("& cmd.exe"))

    def test_custom_linux_script_preserves_arguments_and_failure_log(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            archive_path = root / "source.tar.gz"
            contents = b'printf "%s\\n" "$@"\nexit 23\n'
            with tarfile.open(archive_path, "w:gz") as archive:
                entry = tarfile.TarInfo("scripts/fixture.sh")
                entry.size = len(contents)
                archive.addfile(entry, io.BytesIO(contents))
            digest = hashlib.sha256(archive_path.read_bytes()).hexdigest()
            args = ["space name", "Юникод", "$(touch injected)", "a'b", "", "first\nsecond"]
            def local_remote(record, script):
                return subprocess.run(["bash", "-se"], input=script, check=True,
                                      text=True, capture_output=True)
            with patch.object(runner, "invoke"), patch.object(runner, "remote", local_remote):
                with self.assertRaises(subprocess.CalledProcessError) as result:
                    runner.execute_check({"ip": "fixture", "ssh_argv": ["ssh", "fixture"]},
                                         "linux", "dev", archive_path, digest, str(root),
                                         ["bash", "scripts/fixture.sh", *args], False)
            self.assertEqual(result.exception.returncode, 23)
            self.assertEqual((root / "check.log").read_text(), "\n".join(args) + "\n")
            self.assertFalse((root / "injected").exists())

    def test_compact_test_mode_is_enabled_for_guest_build_wrappers(self):
        for platform in ("linux", "windows"):
            with self.subTest(platform=platform), patch.object(runner, "invoke"), \
                    patch.object(runner, "remote") as remote:
                runner.execute_check({"ip": "fixture", "ssh_argv": ["ssh", "fixture"]},
                                     platform, "dev", Path("fixture"), "hash", "/fixture",
                                     prepare_runtime=False)
                script = remote.call_args.args[1]
                if platform == "windows":
                    encoded = re.search(r"check.cmd', \[Convert\]::FromBase64String\('([^']+)'", script)
                    batch = base64.b64decode(encoded.group(1)).decode()
                    self.assertIn('set "SEVENZIP_COMPACT_TEST_BINARIES=ON"', batch)
                else:
                    self.assertIn("export SEVENZIP_COMPACT_TEST_BINARIES=ON", script)

    def test_long_test_opt_in_is_forwarded_to_both_guest_wrappers(self):
        for platform in ("linux", "windows"):
            for value in ("0", "1"):
                with self.subTest(platform=platform, include_slow=value), \
                        patch.dict(os.environ, {"SEVENZIP_INCLUDE_SLOW_TESTS": value}), \
                        patch.object(runner, "invoke"), patch.object(runner, "remote") as remote:
                    runner.execute_check({"ip": "fixture", "ssh_argv": ["ssh", "fixture"]},
                                         platform, "dev", Path("fixture"), "hash", "/fixture",
                                         prepare_runtime=False)
                    script = remote.call_args.args[1]
                    if platform == "windows":
                        encoded = re.search(r"check.cmd', \[Convert\]::FromBase64String\('([^']+)'", script)
                        batch = base64.b64decode(encoded.group(1)).decode()
                        self.assertIn(f'set "SEVENZIP_INCLUDE_SLOW_TESTS={value}"', batch)
                    else:
                        self.assertIn(f"export SEVENZIP_INCLUDE_SLOW_TESTS={value}", script)

    def test_windows_payload_preserves_shell_metacharacters(self):
        with patch.object(runner, "invoke"), patch.object(runner, "remote") as remote:
            runner.execute_check({"ip": "fixture", "ssh_argv": ["ssh", "fixture"]},
                                 "windows", "dev", Path("fixture"), "hash", "C:/fixture",
                                 ["printf", "%s\\n", "Юникод", "%PATH%", "$(touch injected)", "a b"], False)
        script = remote.call_args.args[1]
        payload = re.search(r"runner-command.sh', \[Convert\]::FromBase64String\('([^']+)'", script)
        decoded = base64.b64decode(payload.group(1)).decode()
        with tempfile.TemporaryDirectory() as temporary:
            result = subprocess.run(["bash", "-se"], input=decoded, cwd=temporary,
                                    text=True, capture_output=True, check=True)
            self.assertEqual(result.stdout, "Юникод\n%PATH%\n$(touch injected)\na b\n")
            self.assertFalse((Path(temporary) / "injected").exists())

    def test_linux_temporary_files_stay_in_snapshot_and_out_of_git(self):
        with tempfile.TemporaryDirectory(prefix="runner space ") as temporary:
            root = Path(temporary)
            archive_path = root / "source.tar.gz"
            with tarfile.open(archive_path, "w:gz") as archive:
                contents = b'printf "%s\\n" "$TEMP" "$TMP" "$TMPDIR"\nmktemp\n'
                entry = tarfile.TarInfo("scripts/fixture.sh")
                entry.size = len(contents)
                archive.addfile(entry, io.BytesIO(contents))
            digest = hashlib.sha256(archive_path.read_bytes()).hexdigest()

            def local_remote(record, script):
                return subprocess.run(["bash", "-se"], input=script, check=True,
                                      text=True, capture_output=True)

            with patch.object(runner, "invoke"), patch.object(runner, "remote", local_remote):
                runner.execute_check({"ip": "fixture", "ssh_argv": ["ssh", "fixture"]},
                                     "linux", "dev", archive_path, digest, str(root),
                                     ["bash", "scripts/fixture.sh"], False)
            lines = (root / "check.log").read_text().splitlines()
            self.assertEqual(lines[:3], [str(root / "runner-tmp")] * 3)
            self.assertEqual(Path(lines[3]).parent, root / "runner-tmp")
            self.assertTrue(Path(lines[3]).is_file())
            self.assertEqual(git(root, "status", "--porcelain"), "")
            self.assertNotIn("runner-tmp", git(root, "ls-files"))

    def test_windows_temp_scope_precedes_extraction_toolchain_and_interactive_launch(self):
        root = "C:/runner/sevenzip-desktop/runs/20261007T060000Z-1234abcd"
        for interactive in (False, True):
            with self.subTest(interactive=interactive), patch.object(runner, "invoke"), \
                    patch.object(runner, "remote") as remote:
                runner.execute_check({"ip": "fixture", "ssh_argv": ["ssh", "fixture"]},
                                     "windows", "dev", Path("source.tar"), "hash", root,
                                     prepare_runtime=False, interactive=interactive)
            script = remote.call_args.args[1]
            temp_scope = "$env:TEMP=$env:TMP=$env:TMPDIR='" + root + "/runner-tmp'"
            self.assertLess(script.index(temp_scope), script.index("tar.exe -xf"))
            encoded = re.search(r"check.cmd', \[Convert\]::FromBase64String\('([^']+)'", script)
            batch = base64.b64decode(encoded.group(1)).decode()
            for variable in ("TEMP", "TMP", "TMPDIR"):
                assignment = f'set "{variable}={root}/runner-tmp"'
                self.assertLess(batch.index(assignment), batch.index("vcvars64.bat"))
            self.assertLess(batch.index("echo /runner-tmp/"), batch.index("git add -A"))
            if interactive:
                child_encoded = re.search(r"-EncodedCommand ([A-Za-z0-9+/=]+)", script)
                child = base64.b64decode(child_encoded.group(1)).decode("utf-16le")
                self.assertLess(child.index('$env:TEMP=$env:TMP=$env:TMPDIR='),
                                child.index("& cmd.exe"))

    def test_linux_headless_default_preserves_explicit_platform(self):
        for requested in (None, "xcb"):
            with self.subTest(requested=requested), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                archive_path = root / "source.tar.gz"
                with tarfile.open(archive_path, "w:gz") as archive:
                    contents = b'printf "%s\\n" "$QT_QPA_PLATFORM"\n'
                    entry = tarfile.TarInfo("scripts/fixture.sh")
                    entry.size = len(contents)
                    archive.addfile(entry, io.BytesIO(contents))
                digest = hashlib.sha256(archive_path.read_bytes()).hexdigest()
                environment = dict(os.environ)
                environment.pop("QT_QPA_PLATFORM", None)
                if requested is not None:
                    environment["QT_QPA_PLATFORM"] = requested

                def local_remote(record, script):
                    return subprocess.run(["bash", "-se"], input=script, check=True,
                                          text=True, capture_output=True, env=environment)

                with patch.object(runner, "invoke"), patch.object(runner, "remote", local_remote):
                    runner.execute_check({"ip": "fixture", "ssh_argv": ["ssh", "fixture"]},
                                         "linux", "dev", archive_path, digest, str(root),
                                         ["bash", "scripts/fixture.sh"], False)
                self.assertEqual((root / "check.log").read_text(),
                                 (requested or "offscreen") + "\n")

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
            (root / "scripts").mkdir()
            (root / "scripts/hyperv-runner.py").write_text("# bundled runner\n")
            (root / "scripts/hyperv_transport.py").write_text("# bundled transport\n")
            (root / "build").mkdir()
            (root / "build/ignored-secret").write_text("excluded")
            destination = Path(temporary) / "source.tar.gz"
            with patch.object(runner, "ROOT", root):
                digest = runner.snapshot(destination, upstream)
            self.assertEqual(digest, hashlib.sha256(destination.read_bytes()).hexdigest())
            with tarfile.open(destination) as archive:
                self.assertEqual(archive.extractfile(name).read(), b"current")
                self.assertEqual(set(archive.getnames()),
                                 {name, "docs/upstream-baseline.json", "upstream.bundle",
                                  "scripts/hyperv-runner.py", "scripts/hyperv_transport.py"})
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
                with patch.object(runner, "discover", return_value={"name": "runner-linux", "ip": "fixture"}), \
                        patch.object(runner, "execute_check", side_effect=error):
                    if expected == "interrupted":
                        with self.assertRaises(KeyboardInterrupt):
                            runner.run_platform("linux", Path(temporary), Path("fixture"),
                                                "fixture-sha", "dev", None, True)
                    else:
                        runner.run_platform("linux", Path(temporary), Path("fixture"),
                                            "fixture-sha", "dev", None, True)
                records = list(Path(temporary).glob("*/run.json"))
                self.assertEqual(len(records), 1)
                record = json.loads(records[0].read_text())
                self.assertEqual(record["status"], expected)
                if expected == "failed":
                    self.assertEqual(record["exit_code"], 17)


if __name__ == "__main__":
    unittest.main()
