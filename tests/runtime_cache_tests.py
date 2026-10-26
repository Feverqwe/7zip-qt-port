"""Offline runtime-cache invalidation and failure recovery regressions."""

import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

SCRIPT = Path(__file__).resolve().parents[1] / "scripts/prepare-runtime.py"
spec = importlib.util.spec_from_file_location("runtime", SCRIPT)
runtime = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runtime)


class RuntimeCacheTests(unittest.TestCase):
    def setUp(self):
        # Windows platform discovery can spawn subprocesses. Keep those probes
        # outside the mocked builder/compiler subprocess boundary.
        for name in ("system", "machine"):
            value = getattr(runtime.platform, name)()
            probe = patch.object(runtime.platform, name, return_value=value)
            probe.start()
            self.addCleanup(probe.stop)

    def test_codec_only_preserves_sfx_and_full_runtime_cache(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            build = root / "build"
            (build / "codec-runtime").mkdir(parents=True)
            (build / "sfx").mkdir()
            sfx = build / "sfx/module"
            sfx.write_bytes(b"retained SFX")
            full_stamp = build / "runtime-cache.json"
            full_stamp.write_bytes(b"retained full-runtime cache")
            def command(args, **kwargs):
                if any("build-sfx" in str(arg) for arg in args):
                    sfx.write_bytes(b"unwanted SFX rebuild")
                (build / "codec-runtime/engine").write_bytes(b"adapted engine")
                return subprocess.CompletedProcess(args, 0)
            with patch.object(runtime, "fingerprint", return_value={"baseline": {"revision": "one"}}), \
                    patch.object(runtime, "prepare_upstream", return_value=root / "sibling"), \
                    patch.object(runtime, "artifact_paths", return_value=["codec-runtime/engine", "sfx/module"]), \
                    patch.object(runtime.subprocess, "run", side_effect=command) as run:
                runtime.prepare(root, root / "sibling", codec_only=True)
                self.assertEqual((build / "codec-runtime/engine").read_bytes(), b"adapted engine")
                self.assertEqual(sfx.read_bytes(), b"retained SFX")
                self.assertEqual(full_stamp.read_bytes(), b"retained full-runtime cache")
                run.reset_mock()
                runtime.prepare(root, root / "sibling", codec_only=True)
                run.assert_not_called()

    def test_missing_pin_uses_local_store_without_changing_sibling(self):
        for sibling_exists in (False, True):
            with self.subTest(sibling_exists=sibling_exists), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                source = root / "official"
                sibling = root / "sibling"
                source.mkdir()

                def git(directory, *args):
                    return subprocess.run(["git", "-C", str(directory), *args], check=True,
                                          capture_output=True).stdout

                git(source, "init", "-q")
                git(source, "config", "user.name", "Fixture")
                git(source, "config", "user.email", "fixture@example.invalid")
                (source / "engine").write_bytes(b"old engine")
                git(source, "add", ".")
                git(source, "commit", "-qm", "old")
                if sibling_exists:
                    git(root, "clone", "-q", str(source), str(sibling))
                    (sibling / "engine").write_bytes(b"user changes")
                    previous_head = git(sibling, "rev-parse", "HEAD")
                    previous_status = git(sibling, "status", "--porcelain")
                (source / "engine").write_bytes(b"pinned engine")
                git(source, "commit", "-qam", "pinned")
                revision = git(source, "rev-parse", "HEAD").decode().strip()
                (source / "engine").write_bytes(b"newer engine")
                git(source, "commit", "-qam", "newer")
                baseline = {"repository": str(source), "revision": revision}
                store = runtime.prepare_upstream(root, sibling, baseline)
                self.assertEqual(git(store, "show", revision + ":engine"), b"pinned engine")
                # Offline reuse must work even when the source is unavailable.
                source.rename(root / "offline")
                reused = runtime.prepare_upstream(root, sibling, baseline)
                self.assertEqual(git(reused, "show", revision + ":engine"), b"pinned engine")
                if sibling_exists:
                    self.assertEqual(git(sibling, "rev-parse", "HEAD"), previous_head)
                    self.assertEqual(git(sibling, "status", "--porcelain"), previous_status)
                    self.assertEqual((sibling / "engine").read_bytes(), b"user changes")
                else:
                    self.assertFalse(sibling.exists())

    def test_relative_runner_upstream_prepares_bare_store_and_publishes_verified_hit(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "fixture-source"
            source.mkdir()
            real_run = subprocess.run
            def git(*args):
                return real_run(["git", "-C", str(source), *args], check=True, capture_output=True).stdout.decode().strip()
            git("init", "-q")
            git("config", "user.name", "Fixture")
            git("config", "user.email", "fixture@example.invalid")
            (source / "source").write_bytes(b"official engine fixture")
            git("add", ".")
            git("commit", "-qm", "baseline")
            revision = git("rev-parse", "HEAD")
            git("branch", "runner-baseline")
            git("bundle", "create", str(root / "upstream.bundle"), "runner-baseline")
            key = {"baseline": {"revision": revision}}
            def command(args, **kwargs):
                if args[0] == "git":
                    return real_run(args, **kwargs)
                # A builder needs the actual pinned source, not just a requested
                # git command. Missing relative-upstream preparation fails here.
                engine = real_run(["git", "-C", str(root / "upstream"), "show", revision + ":source"],
                                  check=True, capture_output=True).stdout
                (root / "build").mkdir(exist_ok=True)
                (root / "build/engine").write_bytes(engine)
                return subprocess.CompletedProcess(args, 0)
            with patch.object(runtime, "fingerprint", return_value=key), \
                    patch.object(runtime, "artifact_paths", return_value=["engine"]), \
                    patch.object(runtime.subprocess, "run", side_effect=command) as run:
                runtime.prepare(root, Path("upstream"))
                self.assertEqual((root / "build/engine").read_bytes(), b"official engine fixture")
                run.reset_mock()
                runtime.prepare(root, Path("upstream"))
                run.assert_not_called()

    def test_missing_changed_or_linked_artifact_invalidates_hit(self):
        with tempfile.TemporaryDirectory() as temporary:
            build = Path(temporary)
            artifact = build / "engine"
            artifact.write_bytes(b"verified engine")
            record = {"fingerprint": {"revision": "one"}, "artifacts": {"engine": runtime.sha(artifact)}}
            self.assertTrue(runtime.cache_valid(build, record, {"revision": "one"}, ["engine"]))
            self.assertFalse(runtime.cache_valid(build, record, {"revision": "two"}, ["engine"]))
            self.assertFalse(runtime.cache_valid(build, record, {"revision": "one"}, ["engine", "module"]))
            artifact.write_bytes(b"different engine")
            self.assertFalse(runtime.cache_valid(build, record, {"revision": "one"}, ["engine"]))
            artifact.unlink()
            self.assertFalse(runtime.cache_valid(build, record, {"revision": "one"}, ["engine"]))
            artifact.write_bytes(b"verified engine")
            with patch.object(Path, "is_symlink", return_value=True):
                self.assertFalse(runtime.cache_valid(build, record, {"revision": "one"}, ["engine"]))

    def test_hit_skips_builders_and_failed_rebuild_does_not_publish_stamp(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            build = root / "build"
            build.mkdir()
            (build / "engine").write_bytes(b"engine")
            key = {"baseline": {"revision": "one"}}
            stamp = build / "runtime-cache.json"
            stamp.write_text(json.dumps({"fingerprint": key, "artifacts": {"engine": runtime.sha(build / "engine")}}))
            with patch.object(runtime, "fingerprint", return_value=key), \
                    patch.object(runtime, "prepare_upstream", return_value=root / "sibling"), \
                    patch.object(runtime, "artifact_paths", return_value=["engine"]), \
                    patch.object(runtime.subprocess, "run") as run:
                runtime.prepare(root, root / "sibling")
                run.assert_not_called()
                (build / "engine").unlink()
                run.side_effect = subprocess.CalledProcessError(17, "builder")
                with self.assertRaises(subprocess.CalledProcessError):
                    runtime.prepare(root, root / "sibling")
                self.assertFalse(stamp.exists())

    def test_fingerprint_changes_with_builder_toolchain_or_baseline(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "scripts").mkdir()
            (root / "docs").mkdir()
            (root / "docs/upstream-baseline.json").write_text('{"revision":"one"}')
            for name in runtime.BUILDERS:
                (root / "scripts" / name).write_text("original")
            probe = subprocess.CompletedProcess("compiler", 0, b"compiler1", b"")
            with patch.object(runtime.subprocess, "run", return_value=probe):
                original = runtime.fingerprint(root)
                for system in ("Linux", "Windows"):
                    with patch.object(runtime.platform, "system", return_value=system):
                        self.assertNotEqual(runtime.fingerprint(root, sanitizer_runtime=True),
                                            runtime.fingerprint(root, sanitizer_runtime=False))
                (root / "scripts" / runtime.BUILDERS[0]).write_text("changed")
                self.assertNotEqual(original, runtime.fingerprint(root))
                (root / "scripts" / runtime.BUILDERS[0]).write_text("original")
                self.assertEqual(original, runtime.fingerprint(root))
                with patch.dict(runtime.os.environ, {"CXXFLAGS": "-changed"}):
                    self.assertNotEqual(original, runtime.fingerprint(root))
                (root / "docs/upstream-baseline.json").write_text('{"revision":"two"}')
                self.assertNotEqual(original, runtime.fingerprint(root))

    def test_partial_exact_update_invalidates_prepared_runtime(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "scripts").mkdir()
            (root / "docs").mkdir()
            source = root / "third_party/7zip/decoder.cpp"
            source.parent.mkdir(parents=True)
            source.write_bytes(b"baseline decoder")
            manifest = {"revision": "one", "exact": [str(source.relative_to(root))]}
            path = root / "docs/upstream-baseline.json"
            path.write_text(json.dumps(manifest))
            for name in runtime.BUILDERS:
                (root / "scripts" / name).write_text("builder")
            with patch.object(runtime.subprocess, "run", return_value=
                              subprocess.CompletedProcess("compiler", 0, b"compiler", b"")):
                before = runtime.fingerprint(root)
                source.write_bytes(b"updated decoder")
                manifest["overrides"] = {str(source.relative_to(root)): "two"}
                path.write_text(json.dumps(manifest))
                updated = runtime.fingerprint(root)
                self.assertNotEqual(before, updated)
                source.write_bytes(b"corrected decoder")
                self.assertNotEqual(updated, runtime.fingerprint(root))


if __name__ == "__main__":
    unittest.main()
