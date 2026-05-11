#!/usr/bin/env python3
"""
strip-private-registry-stanzas.py — remove private-registry package stanzas
from a Yarn 4 lockfile before running flatpak-node-generator.

Usage:
    python3 strip-private-registry-stanzas.py <input-yarn.lock> <output-yarn.lock>

Background
----------
The ProtonMail/WebClients monorepo lockfile contains a handful of packages that
are published only to Proton's private npm registries:

    @proton-meet/*   → https://nexus.protontech.ch/repository/meet-npm/
    @proton/proton-foundation-search
                     → https://nexus.protontech.ch/repository/foundation-npm/
    @tpe/*           → https://gitlab.protontech.ch/.../packages/npm/

These packages are NOT needed by the applications/meet-desktop workspace (the
one we actually build).  flatpak-node-generator would try to resolve them from
the public npm registry and fail with HTTP 404.

This script identifies every top-level Yarn 4 lockfile stanza whose package is:
  1. Not a workspace package (linkType: hard, not soft).
  2. Listed under a private registry scope OR uses __archiveUrl pointing to a
     Proton private server.
  3. Hard-coded known-private packages (proton-foundation-search).

It removes those stanzas and writes the cleaned lockfile to the output path.
The dependency declarations *inside* workspace stanzas that reference the
removed packages are left in place — they are harmless (the generator only
processes top-level stanza keys, not nested values).

To regenerate generated-sources.json after running this script:

    flatpak-node-generator yarn <output-yarn.lock> \
        --max-parallel 16 \
        -o generated-sources.json
"""

import re
import sys


def strip_private_stanzas(input_path: str, output_path: str) -> None:
    with open(input_path, encoding="utf-8") as f:
        lines = f.readlines()

    ranges_to_remove: list[
        tuple[int, int]
    ] = []  # (start, end) 0-indexed, end exclusive

    i = 0
    while i < len(lines):
        line = lines[i]

        # Top-level stanza headers start with a double-quote in column 0.
        if not line.startswith('"'):
            i += 1
            continue

        header = line.strip()
        stanza_start = i
        i += 1

        # Collect the stanza body (indented lines + blank lines).
        while i < len(lines) and (lines[i].startswith(" ") or lines[i].strip() == ""):
            i += 1
        stanza_end = i  # exclusive

        stanza = "".join(lines[stanza_start:stanza_end])

        # Skip workspace packages (linkType: soft).
        if "linkType: soft" in stanza:
            continue

        # Classify as private if any of the following criteria match:
        is_private = (
            # @proton-meet/* scope with npm: protocol
            (re.match(r'^"@proton-meet/', header) and "npm:" in header)
            # @tpe/* scope with npm: protocol
            or (re.match(r'^"@tpe/', header) and "npm:" in header)
            # Package resolved from a Proton private archive URL
            or "__archiveUrl" in stanza
            # Known private @proton/* packages
            or re.match(r'^"@proton/proton-foundation-search@', header)
        )

        if is_private:
            ranges_to_remove.append((stanza_start, stanza_end))

    if not ranges_to_remove:
        print("No private stanzas found — writing unchanged lockfile.")
    else:
        print(f"Removing {len(ranges_to_remove)} private stanza(s):")
        for s, e in ranges_to_remove:
            print(f"  lines {s + 1}–{e}: {lines[s].strip()[:70]}")

    remove_indices: set[int] = set()
    for s, e in ranges_to_remove:
        remove_indices.update(range(s, e))

    new_lines = [l for idx, l in enumerate(lines) if idx not in remove_indices]

    with open(output_path, "w", encoding="utf-8") as f:
        f.writelines(new_lines)

    print(
        f"Done: {len(lines)} lines → {len(new_lines)} lines "
        f"(removed {len(lines) - len(new_lines)})."
    )
    print(f"Written to: {output_path}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(f"Usage: {sys.argv[0]} <input-yarn.lock> <output-yarn.lock>")
    strip_private_stanzas(sys.argv[1], sys.argv[2])
