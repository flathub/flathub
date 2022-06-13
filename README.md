Surge XT
========


This flatpak manifest will build two package: Surge XT as a standalone
and the VSt3 / LV2 plugins.

The standalone is built at the same time as the VST3.

LV2 need a separate build

## Filesystem access

Surge XT need `xdg-documents/Surge XT` for user data. If there is no
`xdg-documents` then it will try `~/.Surge XT` as a fallback. This is
also used by the plugin.

`~/.config` is needed to save a settings files by both Surge XT and
Surge XT Effects.
