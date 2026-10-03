"""Validate the built app's association declaration without OS registration."""
import pathlib
import plistlib
import re
import sys

bundle = pathlib.Path(sys.argv[1])
resource = pathlib.Path(sys.argv[2])
with (bundle / "Contents" / "Info.plist").open("rb") as stream:
    info = plistlib.load(stream)
assert info["CFBundleIdentifier"] == "org.sevenzip-desktop"
assert info["CFBundlePackageType"] == "APPL"
assert (bundle / "Contents" / "MacOS" / info["CFBundleExecutable"]).is_file()
source = re.search(r'\b100\s+"([^"]+)"', resource.read_text()).group(1)
expected = {pair.split(":")[0] for pair in source.split()}
declared = set()
for document in info["CFBundleDocumentTypes"]:
    assert document["CFBundleTypeRole"] == "Viewer"
    assert document["LSHandlerRank"] == "Alternate"
    declared.update(document["CFBundleTypeExtensions"])
assert declared == expected, (declared - expected, expected - declared)
assert {"7z", "zip", "tar", "gz", "tgz", "wim"} <= declared
print("Built app declares the source archive extensions without changing OS defaults")
