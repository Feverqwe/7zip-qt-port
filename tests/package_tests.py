"""Runtime package boundaries and release version behavior, with local fixtures."""
import hashlib
import importlib.util
import io
from pathlib import Path
import tarfile
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]


def load_script(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / f"scripts/{name}.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


package = load_script("package")
release = load_script("release")


class PackageTests(unittest.TestCase):
    def test_windows_copies_transitive_crt_only_without_installer(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / "package"
            runtime = Path(temporary) / "crt"
            (root / "bin").mkdir(parents=True)
            runtime.mkdir()
            (root / "bin/app.exe").write_bytes(b"app")
            (root / "bin/vc_redist.x64.exe").write_bytes(b"installer")
            for name in ("msvcp140.dll", "vcruntime140.dll", "unused.dll"):
                (runtime / name).write_bytes(name.encode())
            dependencies = {"app.exe": ["msvcp140.dll", "kernel32.dll"],
                            "msvcp140.dll": ["vcruntime140.dll"]}
            with patch.object(package, "imports", side_effect=lambda path: dependencies.get(path.name, [])):
                package.deploy_crt(root, runtime)
            self.assertEqual(sorted(path.name for path in (root / "bin").iterdir()),
                             ["app.exe", "msvcp140.dll", "vcruntime140.dll"])

    def test_rejects_sources_build_metadata_and_nested_archives(self):
        with tempfile.TemporaryDirectory(prefix="package 空 ") as temporary:
            root = Path(temporary)
            for name in ("code.cpp", "Source/source.txt", "CMakeCache.txt", "source.tar.xz"):
                with self.subTest(name=name):
                    path = root / name
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_text("fixture")
                    with self.assertRaises(ValueError):
                        package.validate_payload(root)
                    path.unlink()
                    if path.parent != root:
                        path.parent.rmdir()

    def test_prunes_development_preserves_runtime_notices(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for name in ("Headers/qt.h", "runtime.dylib", "Notices/LICENSE.txt",
                         "libqoffscreen.dylib", "imageformats/libqjpeg.dylib",
                         "imageformats/libqico.dylib"):
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text("fixture")
            package.prune_development(root)
            package.validate_payload(root)
            self.assertEqual(sorted(path.relative_to(root).as_posix() for path in root.rglob("*")
                                    if path.is_file()), ["Notices/LICENSE.txt",
                                                         "imageformats/libqico.dylib", "runtime.dylib"])

    def test_rejects_escaping_symlink_preserves_alias(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / "runtime"
            root.mkdir()
            (root / "app").write_bytes(b"runtime")
            try:
                (root / "alias").symlink_to("app")
            except OSError:
                self.skipTest("Native symbolic links are unavailable")
            package.validate_payload(root)
            (root / "escape").symlink_to("../outside")
            with self.assertRaises(ValueError):
                package.validate_payload(root)

    def test_qt_archive_extracts_only_licenses_and_checks_version_hash(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            archive = root / "qtbase-everywhere-src-6.12.0.tar.xz"
            with tarfile.open(archive, "w:xz") as stream:
                for name in ("LICENSES/LGPL-3.0-only.txt", "src/code.cpp"):
                    member = tarfile.TarInfo("qtbase-everywhere-src-6.12.0/" + name)
                    member.size = 7
                    stream.addfile(member, io.BytesIO(b"fixture"))
            checksum = root / "checksum"
            checksum.write_text(hashlib.sha256(archive.read_bytes()).hexdigest())
            output = root / "notices"
            package.qt_notices(archive, checksum, "6.12.0", output)
            self.assertTrue((output / "LICENSES/LGPL-3.0-only.txt").is_file())
            self.assertFalse((output / "src").exists())
            with self.assertRaises(ValueError):
                package.qt_notices(archive, checksum, "6.11.2", output)
            checksum.write_text("0" * 64)
            with self.assertRaises(ValueError):
                package.qt_notices(archive, checksum, "6.12.0", output)

    def test_release_versions_and_tag_match_without_git_writes(self):
        for choice, expected in (("patch", "0.1.1"), ("minor", "0.2.0"),
                                 ("major", "1.0.0"), ("1.2.3", "1.2.3")):
            self.assertEqual(release.next_version("0.1.0", choice), expected)
        for choice in ("0.1.0", "0.0.1", "01.2.3", "nonsense"):
            with self.assertRaises(ValueError):
                release.next_version("0.1.0", choice)
        release.check_tag("v" + release.current_version())
        with self.assertRaises(ValueError):
            release.check_tag("v999.0.0")


if __name__ == "__main__":
    unittest.main()
