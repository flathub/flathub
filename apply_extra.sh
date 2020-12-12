#!/usr/bin/sh
ar x edge.deb data.tar.xz
rm edge.deb
tar -xvf data.tar.xz --strip-components=4 ./opt/microsoft/msedge-dev
rm data.tar.xz nacl*
cp /app/bin/stub_sandbox msedge-sandbox
