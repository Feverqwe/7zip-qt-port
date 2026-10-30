#!/usr/bin/env python3
"""Expand pinned local dependencies inside an owned build directory."""

import argparse
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.dependencies.source_archives import SourceArchives


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--destination", type=Path, required=True)
    args = parser.parse_args()
    try:
        SourceArchives(ROOT).prepare(args.destination)
    except (OSError, ValueError) as error:
        parser.exit(1, "dependency-sources: " + str(error) + "\n")


if __name__ == "__main__":
    main()
