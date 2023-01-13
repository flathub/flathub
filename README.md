BYOD
====


Flatpaking notes

Checking out the repository with submodules doesn't work because one
of the submodule has an orphan commit and it not fetched.
See https://github.com/Chowdhury-DSP/chowdsp_juce_dsp/issues/1

So `byod.json` has been auto generated to have all the submodule but
the one that is a problem, that is downloaded as a tarball.

Permissions
-----------

DRI: The app use GL to render.

Filesystem:

`--filesystem=xdg-run/pipewire-0`: needed for Pipewire

`--filesystem=~/.config/ChowdhuryDSP`: To persist and share with the
plugin. Doesn't honour XDG
`--persist=.config` because of `./.config/BYOD.settings`.
