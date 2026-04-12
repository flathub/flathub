#!/usr/bin/env python3
"""
strip-private-registry-lockfile.py

Reads a Yarn Berry (v2+) yarn.lock and writes a copy with any package
blocks whose `resolution:` URL points to a non-public registry removed.

Usage:
    python3 strip-private-registry-lockfile.py [OPTIONS] <input> [output]

Arguments:
    input   Path to the source yarn.lock (default: yarn.lock)
    output  Path to write the filtered lockfile (default: yarn.lock.filtered)

Options:
    --private-scope SCOPE   npm scope to treat as private (e.g. @tpe).
                            May be given multiple times.  When no scopes are
                            given the script infers private scopes from any
                            `archiveUrl` or `npmRegistryServer` in
                            .yarnrc.yml that is not registry.yarnpkg.com or
                            registry.npmjs.org.
    --dry-run               Print what would be removed without writing output.
    -v / --verbose          Print the removed package identifiers.
"""

import argparse
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Heuristics for "private" resolution URLs
# ---------------------------------------------------------------------------

PUBLIC_REGISTRIES = {
    "registry.yarnpkg.com",
    "registry.npmjs.org",
}


def _is_private_resolution(resolution_line: str) -> bool:
    """Return True when the resolution URL points to a non-public host."""
    # Berry encodes the archive URL as a query parameter:
    # resolution: "@tpe/quark-client@npm:1.2.1::__archiveUrl=https%3A%2F%2F..."
    # or sometimes as a plain URL suffix for patched packages.
    match = re.search(r"__archiveUrl=([^\"'\s]+)", resolution_line)
    if match:
        raw = match.group(1)
        # URL-decode the % sequences for the host check
        from urllib.parse import unquote

        decoded = unquote(raw)
        for host in PUBLIC_REGISTRIES:
            if host in decoded:
                return False
        return True  # archiveUrl points somewhere non-public

    # No archiveUrl – fall back to scope check in the header
    return False


def _resolution_is_private_scope(header: str, private_scopes: set[str]) -> bool:
    """Return True when the package's scope is in private_scopes."""
    for scope in private_scopes:
        # header looks like: "@tpe/quark-client@npm:1.2.1":
        if header.lstrip('"').startswith(scope + "/"):
            return True
    return False


# ---------------------------------------------------------------------------
# Lockfile block parser
# ---------------------------------------------------------------------------


def parse_blocks(text: str) -> list[tuple[int, int, str]]:
    """
    Return a list of (start_line, end_line, header) for every package block.
    Lines are 0-based, end_line is exclusive (like range()).
    The preamble (comments at the top) is returned as a block with header=''.
    """
    lines = text.splitlines(keepends=True)
    blocks: list[tuple[int, int, str]] = []
    i = 0
    n = len(lines)

    # Collect preamble: leading blank lines and comments
    preamble_start = 0
    while i < n and (lines[i].startswith("#") or lines[i].strip() == ""):
        i += 1
    if i > preamble_start:
        blocks.append((preamble_start, i, ""))

    # Parse package blocks
    while i < n:
        line = lines[i]
        # A new package block starts with a quoted identifier (no indentation)
        if line and not line[0].isspace() and line.strip() and not line.startswith("#"):
            header = line.strip().rstrip(":")
            block_start = i
            i += 1
            # The block continues as long as lines are indented or blank
            while i < n:
                l = lines[i]
                if l.strip() == "":
                    # blank separator – end of block
                    i += 1
                    break
                if l[0].isspace() or l.startswith(" "):
                    i += 1
                else:
                    break
            blocks.append((block_start, i, header))
        else:
            # lone blank or comment line outside a block – attach to next
            i += 1

    return blocks


# ---------------------------------------------------------------------------
# Main filtering logic
# ---------------------------------------------------------------------------


def filter_lockfile(
    src: str,
    private_scopes: set[str],
    dry_run: bool = False,
    verbose: bool = False,
) -> tuple[str, list[str]]:
    """
    Remove private-registry blocks from *src* (the full lockfile text).

    Returns (filtered_text, removed_headers).
    """
    lines = src.splitlines(keepends=True)
    blocks = parse_blocks(src)

    keep_ranges: list[tuple[int, int]] = []
    removed: list[str] = []

    for start, end, header in blocks:
        if not header:
            # preamble – always keep
            keep_ranges.append((start, end))
            continue

        block_text = "".join(lines[start:end])

        # Check 1: resolution URL points to a private archiveUrl
        private = False
        for l in lines[start:end]:
            if "resolution:" in l and _is_private_resolution(l):
                private = True
                break

        # Check 2: the package scope is explicitly marked private
        if not private and private_scopes:
            if _resolution_is_private_scope(header, private_scopes):
                private = True

        if private:
            removed.append(header)
            if verbose:
                print(f"  REMOVE  {header}", file=sys.stderr)
        else:
            keep_ranges.append((start, end))

    # Reconstruct the filtered text
    out_lines: list[str] = []
    for start, end in keep_ranges:
        out_lines.extend(lines[start:end])

    return "".join(out_lines), removed


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument(
        "input",
        nargs="?",
        default="yarn.lock",
        help="Source yarn.lock (default: yarn.lock)",
    )
    ap.add_argument(
        "output",
        nargs="?",
        default=None,
        help="Output path (default: <input>.filtered)",
    )
    ap.add_argument(
        "--private-scope",
        dest="private_scopes",
        action="append",
        default=[],
        metavar="SCOPE",
        help="npm scope to treat as private, e.g. --private-scope @tpe",
    )
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    src_path = Path(args.input)
    if not src_path.exists():
        print(f"error: {src_path} not found", file=sys.stderr)
        return 1

    out_path = Path(args.output) if args.output else src_path.with_suffix(".filtered")

    private_scopes = set(args.private_scopes)

    # Auto-detect private scopes from .yarnrc.yml if present
    yarnrc = src_path.parent / ".yarnrc.yml"
    if yarnrc.exists():
        yarnrc_text = yarnrc.read_text()
        # Find npmScopes entries that reference non-public registries
        for scope_match in re.finditer(
            r"^\s{2,4}(\S+):\s*$.*?npmRegistryServer:\s*['\"]([^'\"]+)['\"]",
            yarnrc_text,
            re.MULTILINE | re.DOTALL,
        ):
            scope_name = scope_match.group(1).strip()
            registry = scope_match.group(2).strip()
            is_public = any(h in registry for h in PUBLIC_REGISTRIES)
            if not is_public:
                full_scope = (
                    f"@{scope_name}" if not scope_name.startswith("@") else scope_name
                )
                private_scopes.add(full_scope)
                if args.verbose:
                    print(
                        f"  auto-detected private scope: {full_scope} → {registry}",
                        file=sys.stderr,
                    )

    if not private_scopes and args.verbose:
        print(
            "  note: no private scopes detected; "
            "relying solely on archiveUrl heuristic",
            file=sys.stderr,
        )

    src_text = src_path.read_text(encoding="utf-8")

    filtered, removed = filter_lockfile(
        src_text,
        private_scopes,
        dry_run=args.dry_run,
        verbose=args.verbose,
    )

    print(
        f"Removed {len(removed)} private package block(s) from {src_path}.",
        file=sys.stderr,
    )
    if removed and not args.verbose:
        for h in removed:
            print(f"  - {h}", file=sys.stderr)

    if args.dry_run:
        print("Dry-run mode: no output written.", file=sys.stderr)
        return 0

    out_path.write_text(filtered, encoding="utf-8")
    print(f"Filtered lockfile written to {out_path}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
