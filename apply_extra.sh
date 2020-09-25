#!/usr/bin/sh

main() {
ar x brave.deb data.tar.xz; rm -f brave.deb
tar -xvf data.tar.xz --strip-components=4 ./opt/brave.com/brave; rm -f data.tar.xz
}
main 2>/dev/null >/dev/null
