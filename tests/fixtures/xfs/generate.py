#!/usr/bin/env python3
"""Create deterministic-content XFS samples with xfsprogs, without mounting.

Run on Linux with mkfs.xfs. The UUID and timestamps are filesystem metadata;
tests assert file data/paths, not the generator's incidental metadata bytes.
"""
import argparse
import pathlib
import subprocess
import tempfile
import zlib


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    print(subprocess.check_output(["mkfs.xfs", "-V"], text=True).strip())
    with tempfile.TemporaryDirectory(prefix="sevenzip-xfs-") as temp:
        root = pathlib.Path(temp)
        payload = root / "payload"
        payload.write_bytes(("XFS: файл 空\n".encode() * 5000) + b"tail\n")
        empty = root / "empty"
        empty.touch()
        proto = root / "prototype"
        rows = ["dummy", "0 0", "d--755 0 0",
                f"файл_空.bin ---644 1000 1000 {payload}",
                "nested d--755 1000 1000",
                f"with-space.bin ---644 1000 1000 {payload}",
                f"empty ---644 1000 1000 {empty}",
                "link l--777 1000 1000 with-space.bin",
                # xfsprogs 6.6 prototype writes raw bytes for remote links;
                # 200 bytes exercises v4 remote storage but stays inline in v5.
                "long-link l--777 1000 1000 " + "a" * 200, "$",
                "many d--755 1000 1000"]
        rows += [f"entry-{i:04d} ---644 1000 1000 {empty}" for i in range(600)]
        rows += ["$", "$"]
        proto.write_text("\n".join(rows) + "\n", encoding="utf-8")
        for version in (4, 5):
            image = root / "image"
            subprocess.run(["mkfs.xfs", "-f", "-d", f"file,name={image},size=320m",
                            "-m", "crc=0,bigtime=0,reflink=0" if version == 4 else "crc=1,bigtime=1",
                            "-p", str(proto)], check=True, stdout=subprocess.DEVNULL)
            compressor = zlib.compressobj(9)
            with image.open("rb") as source, (args.output / f"v{version}.zlib").open("wb") as out:
                while chunk := source.read(1024 * 1024):
                    out.write(compressor.compress(chunk))
                out.write(compressor.flush())
            image.unlink()


if __name__ == "__main__":
    main()
