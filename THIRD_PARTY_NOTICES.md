# Licenses and third-party notices

The root [MIT license](LICENSE) covers original project work unless a file or
component specifies another license. It does not relicense imported or adapted
sources. Retain their copyright and license notices when redistributing them.

| Component | License and source information |
| --- | --- |
| 7-Zip sources, adapted UI, language files, help and resources | [7-Zip license](third_party/7zip/License.txt): LGPL-2.1-or-later, public domain and the listed BSD exceptions; RAR decoder code also carries the unRAR restriction. [Source audit](docs/upstream-port-audit.md) and [baseline](docs/upstream-baseline.json). |
| Read-only exFAT/TexFAT module and its Linux adaptation | GPL-2.0-or-later; [license text](third_party/linux/LICENSES/preferred/GPL-2.0), retained SPDX notices and [source audit](docs/upstream-port-audit/exfat-linux.md). |
| Linux-derived cramfs read module | GPL-2.0-only; [license text](third_party/linux/LICENSES/preferred/GPL-2.0), retained Linux notices and [source audit](docs/upstream-port-audit/cramfs-linux.md). The separate format module reuses LGPL 7-Zip handler/decoder/ABI sources; it contains no unRAR implementation. |
| Linux-derived FAT compatibility module | GPL-2.0-only; [license text](third_party/linux/LICENSES/preferred/GPL-2.0), retained FAT/NLS notices and [source audit](docs/upstream-port-audit/fat-linux.md). The separate module reuses the LGPL 7-Zip handler and archive ABI; it contains no unRAR implementation. |
| Linux NTFS3 read module and LZNT1/LZX/XPRESS decoders | GPL-2.0-only as a combined module; [license text](third_party/linux/LICENSES/preferred/GPL-2.0), retained Paragon Software and Eric Biggers notices and [source audit](docs/upstream-port-audit/ntfs-linux.md). The original LZX/XPRESS library remains GPL-2.0-or-later. The separate format module reuses LGPL 7-Zip archive/stream sources and contains no unRAR implementation. |
| Linux HFS+ Unicode and hardlink read module | GPL-2.0-only as a combined module; [license text](third_party/linux/LICENSES/preferred/GPL-2.0), retained Brad Boyer/Ardis notices and [source audit](docs/upstream-port-audit/hfs-linux.md). Reused 7-Zip archive sources retain LGPL notices; LZFSE/LZVN retains [Apple/Igor Pavlov BSD terms](third_party/lzfse/LICENSE). The separate module contains no unRAR implementation. |
| Linux-derived EROFS read module | GPL-2.0-only; [license text](third_party/linux/LICENSES/preferred/GPL-2.0) and [source audit](docs/upstream-port-audit/erofs-linux.md). Linux LZ4 retains Yann Collet's copyright (2011–2016) and [BSD-2-Clause terms](third_party/linux/LICENSES/preferred/BSD-2-Clause); XZ Embedded retains Lasse Collin/Igor Pavlov attribution and [0BSD terms](third_party/linux/LICENSES/deprecated/0BSD). Reused 7-Zip decoder/ABI sources retain their notices. The separate module contains no unRAR implementation. |
| Brotli decoder and 7-Zip codec adaptation | [Brotli MIT license](third_party/brotli/LICENSE) and [7zip-zstd LGPL notice](third_party/brotli/7zip-zstd-LICENSE.txt); see the [source audit](docs/upstream-port-audit.md). |
| Tooling adapted from htopQt | GPL-2.0-or-later; [retained project notice](third_party/htopqt/LICENSE), [GPL text](third_party/linux/LICENSES/preferred/GPL-2.0). Includes the borrowed release, packaging, Qt installation/source-fetching and Hyper-V tooling described in [CI](docs/ci.md) and [runner provenance](docs/hyperv-runners.md#runner-configuration-and-evidence). |
| libarchive RAR test data | [Retained license](tests/fixtures/rar/libarchive-COPYING.txt) and [fixture provenance](tests/fixtures/rar/README.md). Test data is not shipped as an application dependency. |
| Lucide command/navigation icons | SVG geometry adapted to native Qt paths; [ISC and Feather MIT notices](third_party/lucide/LICENSE), [source provenance](docs/upstream-port-audit/ui-redesign.md#итерация-22-контурные-значки-команд). |

Qt and other runtime dependencies keep their own licenses.
Qt Base translations are merged from the matching SDK; the corresponding official
[Qt Translations source](https://download.qt.io/official_releases/qt/6.12/6.12.0/submodules/qttranslations-everywhere-src-6.12.0.tar.xz)
and source provenance are recorded in the [localization audit](docs/upstream-port-audit/qt-linguist.md).
Runtime packages
include dependency notices; packaging details and signing limitations are in
[CI and packaging](docs/ci.md). The repository's MIT license alone does not
describe the licensing of an entire binary distribution.
