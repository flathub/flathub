#!/usr/bin/env python3
"""Keep node-sources.json limited to what the authenticator build needs.

  prune-node-sources.py lock <yarn.lock> <cache-dir>
      Print the lockfile entries for the packages a focused install fetched
      into <cache-dir>, plus the arm64 builds of any native linux packages.

  prune-node-sources.py sources <node-sources.json>
      Drop the Playwright browser downloads (test-only) in place.
"""
import json
import os
import re
import sys


def lock(lockfile, cache_dir):
    fetched = set()
    for name in os.listdir(cache_dir):
        m = re.match(r"^(.+)-npm-(.+)-[0-9a-f]{10}-[0-9a-f]+\.zip$", name)
        if m:
            fetched.add((m.group(1), m.group(2)))

    def wanted(ident, version):
        slug = ident.replace("/", "-")
        if (slug, version) in fetched:
            return True
        x64 = re.sub(r"arm64|aarch64", "x64", slug)
        return x64 != slug and "linux" in slug and (x64, version) in fetched

    blocks = open(lockfile).read().split("\n\n")
    out = []
    for block in blocks:
        header = block.lstrip("\n").split("\n", 1)[0]
        if header.startswith(("#", "__metadata")):
            out.append(block)
            continue
        m = re.search(r'^  resolution: "(@?[^@"]+)@npm:([^"]+)"$', block, re.M)
        if m and wanted(m.group(1), m.group(2)):
            out.append(block)
    sys.stdout.write("\n\n".join(out))


def sources(path):
    data = json.load(open(path))
    data = [s for s in data if "ms-playwright" not in json.dumps(s)]
    with open(path, "w") as f:
        json.dump(data, f, indent=4)
        f.write("\n")


if __name__ == "__main__":
    if len(sys.argv) == 4 and sys.argv[1] == "lock":
        lock(sys.argv[2], sys.argv[3])
    elif len(sys.argv) == 3 and sys.argv[1] == "sources":
        sources(sys.argv[2])
    else:
        sys.exit(__doc__)
