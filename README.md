About Ardour flatpak
====================

Some notes about this Flatpak.

Plugins support
---------------

The package supports LV2, LADSPA, DSSI and Linux VST/VST3 plugins
built for Flatpak. You cannot install or use plugins not packaged as a
Flatpak. There is no support for Win32 emulated VST either.

Before upgrading the runtime used, please consider that the plugins
need to be upgraded first. Provision is made for plugins to live in
branches as to be available for different runtimes.

Jack audio support
------------------

Ardour is build without Jack Audio support because Jack doesn't work
from inside Flatpak.

When PipeWire-Jack works we can consider the option.

ALSA Device reservation
-----------------------

Ardour requires exclusive use of the ALSA sound device. To that effect
it supports the device reservation D-Bus interface, hence the
`--own-name=org.freedesktop.ReserveDevice1.*` permission.
