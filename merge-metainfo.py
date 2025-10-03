#!/usr/bin/env python3
import argparse
from pathlib import Path
from xml.etree import ElementTree


def merge(a: Path, b: Path):
    merged = ElementTree.parse(a).getroot()
    merged.extend(ElementTree.parse(b).getroot())
    with open(a, "wb+") as f:
        f.write(ElementTree.tostring(merged))


def _main():
    parser = argparse.ArgumentParser()
    parser.add_argument("a", type=Path)
    parser.add_argument("b", type=Path)
    args = parser.parse_args()
    merge(args.a, args.b)


if __name__ == "__main__":
    _main()
