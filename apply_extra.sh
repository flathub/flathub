#!/usr/bin/sh
ar x chrome.deb data.tar.xz
rm chrome.deb
tar -xf data.tar.xz --strip-components=4 ./opt/google/chrome-unstable
rm data.tar.xz nacl*
cp /app/bin/stub_sandbox chrome-sandbox
