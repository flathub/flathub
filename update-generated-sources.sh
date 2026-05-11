#!/usr/bin/env bash
# update-generated-sources.sh — regenerate generated-sources.json locally.
#
# This script replicates what the GitHub Actions workflow does, so you can run
# it on your own machine after bumping the upstream tag in me.proton.Meet.yml
# (or when you want to regenerate the sources for any other reason).
#
# Requirements
# ────────────
#   git             — to clone the upstream repo
#   python3         — to run strip-private-registry-stanzas.py
#   flatpak-node-generator — install with:
#                       pip install flatpak-node-generator
#                     or via pipx:
#                       pipx install flatpak-node-generator
#
# Usage
# ─────
#   # Use the tag currently in me.proton.Meet.yml:
#   ./update-generated-sources.sh
#
#   # Override the tag explicitly:
#   ./update-generated-sources.sh proton-meet@0.4.17.0
#
# After running, review the diff with:
#   git diff generated-sources.json

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MANIFEST="$SCRIPT_DIR/me.proton.Meet.yml"
STRIP_SCRIPT="$SCRIPT_DIR/strip-private-registry-stanzas.py"

# ── Resolve the upstream tag ──────────────────────────────────────────────────
if [ -n "${1:-}" ]; then
    TAG="$1"
    echo "==> Using tag from command-line argument: $TAG"
else
    echo "==> Reading tag from $MANIFEST..."
    TAG=$(python3 - <<'PYEOF'
import re, sys
text = open("me.proton.Meet.yml").read()
m = re.search(r'type:\s*git.*?tag:\s*([^\s#\n]+)', text, re.DOTALL)
if m:
    print(m.group(1).strip("'\""))
else:
    sys.exit("ERROR: Could not find a 'tag:' field under 'type: git' in me.proton.Meet.yml")
PYEOF
    )
    echo "==> Resolved tag: $TAG"
fi

# ── Prerequisite checks ───────────────────────────────────────────────────────
if ! command -v flatpak-node-generator &>/dev/null; then
    echo ""
    echo "ERROR: flatpak-node-generator not found in PATH."
    echo "Install it with one of:"
    echo "    pip install flatpak-node-generator"
    echo "    pipx install flatpak-node-generator"
    exit 1
fi

if ! command -v git &>/dev/null; then
    echo "ERROR: git not found in PATH."
    exit 1
fi

# ── Clone upstream repo ───────────────────────────────────────────────────────
TMPDIR=$(mktemp -d)
trap 'echo "==> Cleaning up $TMPDIR"; rm -rf "$TMPDIR"' EXIT

echo ""
echo "==> Cloning ProtonMail/WebClients at tag '$TAG'..."
echo "    (shallow clone — this may still take a minute)"
git clone \
    --depth=1 \
    --branch "$TAG" \
    https://github.com/ProtonMail/WebClients \
    "$TMPDIR/WebClients"

# ── Strip private-registry stanzas ───────────────────────────────────────────
echo ""
echo "==> Stripping private-registry stanzas from yarn.lock..."
python3 "$STRIP_SCRIPT" \
    "$TMPDIR/WebClients/yarn.lock" \
    "$TMPDIR/yarn-public.lock"

# ── Generate sources ─────────────────────────────────────────────────────────
echo ""
echo "==> Generating generated-sources.json..."
echo "    (this typically takes 5–15 minutes — packages are downloaded from npm)"
flatpak-node-generator yarn "$TMPDIR/yarn-public.lock" \
    --max-parallel 16 \
    -o "$SCRIPT_DIR/generated-sources.json"

# ── Done ──────────────────────────────────────────────────────────────────────
echo ""
echo "==> Done!  generated-sources.json has been updated."
echo "    Review changes with:  git diff generated-sources.json"
echo "    Commit when satisfied: git add generated-sources.json && git commit -m 'chore: regenerate generated-sources.json for $TAG'"
