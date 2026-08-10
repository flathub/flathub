#!/usr/bin/env bash

# Copyright (C) 2026 imngkhang
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.fsf.org/licenses/>.

# Here we will extract the ClassIn .deb package.
cd /app/extra
DEB_FILE=""
if [ -f classin-amd64.deb ]; then
    DEB_FILE="classin-amd64.deb"
elif [ -f classin-arm64.deb ]; then
    DEB_FILE="classin-arm64.deb"
fi

if [ -n "$DEB_FILE" ]; then
    echo "Extracting $DEB_FILE..."
    ar x "$DEB_FILE"
    rm -f "$DEB_FILE"
else
    echo "Error: No .deb file found in /app/extra!" >&2
    exit 1
fi

# Clean up old extraction to prevent conflict/bloat on updates
rm -rf /app/extra/opt

if [ -f data.tar.xz ]; then
    tar -xf data.tar.xz 
elif [ -f data.tar.zst ]; then
    tar --zstd -xf data.tar.zst 
elif [ -f data.tar.gz ]; then
    tar -xf data.tar.gz 
fi
# Remove temp files
rm -f control.tar.* data.tar.* debian-binary

