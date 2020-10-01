#!/usr/bin/sh

main() {
ar x brave.deb data.tar.xz; rm -f brave.deb
tar -xvf data.tar.xz --strip-components=4 ./opt/brave.com/brave
#tar -xvf data.tar.xz --strip-components=4 ./usr/share/applications/brave-browser.desktop
rm -f data.tar.xz
#sed -e 's/Exec=\/usr\/bin\/brave-browser-stable/Exec=\/app\/bin\/brave/g' -i brave-browser.desktop
#sed -e 's/Icon=brave-browser/Icon=com.brave.Browser/g' -i brave-browser.desktop
}
main 2>/dev/null >/dev/null
