#!/usr/bin/sh
ar x chrome.deb data.tar.xz
rm chrome.deb
tar -xvf data.tar.xz --strip-components=4 ./opt/google/chrome
rm data.tar.xz nacl*
cp /app/bin/stub_sandbox chrome-sandbox
