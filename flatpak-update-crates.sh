#!/bin/bash
#
# Update the manifest for rust crates used by the VPN project.
#
SRCDIR=$(dirname $0)

# Create a temporary directory for the checkouts
TMPDIR=$(mktemp -d)
function cleanup(){ rm -rf $TMPDIR; }
trap cleanup EXIT

# Parse a flatpak manifest and determine the Cargo.lock URL
# The path to the manifest should be provided as an argument.
function parse_cargo_url() {
python - << EOF
import sys, yaml, json

def treeish(gitsrc):
    if 'commit' in gitsrc:
        return gitsrc['commit']
    if 'tag' in gitsrc:
        return gitsrc['tag']
    if 'branch' in gitsrc:
        return gitsrc['branch']
    return 'main'

with open("$1", 'r') as fp:
    for x in yaml.load(fp, Loader=yaml.SafeLoader)['modules'][-1]['sources']:
        if not isinstance(x, dict):
            continue
        if x['type'] != 'git':
            continue

        print(f"{x['url']}/raw/{treeish(x)}/Cargo.lock")
        exit(0)

print("Unable to parse git URL from $1", file=sys.stderr)
exit(1)
EOF
}

# Grab the latest flatpak builder tools
curl -sSL https://github.com/flatpak/flatpak-builder-tools/raw/master/cargo/flatpak-cargo-generator.py -o $TMPDIR/flatpak-cargo-generator.py

if [ $# -ge 1 ]; then
  # The caller can provide a Cargo.lock file as an argument
  echo "Generating dependencies from $1" >&2
  CARGO_LOCK_FILE=$1
else
  # Otherwise - we need to download the Cargo.lock from the source
  CARGO_LOCK_URL=$(parse_cargo_url ${SRCDIR}/org.mozilla.vpn.yml)
  CARGO_LOCK_FILE=$TMPDIR/Cargo.lock
  echo "Generating dependencies from $CARGO_LOCK_URL" >&2
  curl -sSL $CARGO_LOCK_URL -o $CARGO_LOCK_FILE
fi

# Generate the cargo dependencies and convert to YAML
python $TMPDIR/flatpak-cargo-generator.py -o ${SRCDIR}/flatpak-vpn-crates.json $CARGO_LOCK_FILE
