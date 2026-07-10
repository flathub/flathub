#!/bin/sh
# Runs on the user's machine at install time (extra-data), inside the
# freedesktop Platform runtime. The runtime ships no ar/dpkg, so the .deb
# (an ar(5) archive) is unpacked with python.
set -e

python3 - <<'PYEOF'
import os
import sys

with open('claude.deb', 'rb') as f:
    if f.read(8) != b'!<arch>\n':
        sys.exit('claude.deb: not an ar archive')
    while True:
        hdr = f.read(60)
        if len(hdr) < 60:
            sys.exit('claude.deb: data.tar member not found')
        name = hdr[0:16].decode('ascii', 'replace').strip()
        size = int(hdr[48:58])
        if name.startswith('data.tar'):
            # basename: never let an archive member name escape the cwd
            with open(os.path.basename(name), 'wb') as out:
                left = size
                while left > 0:
                    chunk = f.read(min(1 << 20, left))
                    if not chunk:
                        sys.exit('claude.deb: truncated archive')
                    out.write(chunk)
                    left -= len(chunk)
            break
        f.seek(size + (size & 1), 1)
PYEOF

tar -xf data.tar.*
mv usr/lib/claude-desktop claude-desktop
rm -rf usr data.tar.* claude.deb
