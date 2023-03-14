#!/usr/bin/env sh
# https://github.com/flathub/org.gnome.NetworkDisplays/blob/5a3369d04e32ccd092e42463de5171f473a8601d/files/polkit/autogen.sh

gtkdocize --flavour no-tmpl
autoreconf -if
