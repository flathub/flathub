#!/bin/sh
set -e
# Runs once at install time. Flatpak places the downloaded payload in /app/extra
# and calls this with that as the working directory. Unpack it and drop the archive.
tar -xf aiche-payload.tar.gz
rm aiche-payload.tar.gz
