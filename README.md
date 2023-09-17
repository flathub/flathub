LightZone
=========

Flatpak for LightZone.

https://github.com/ktgw0316/LightZone

Uses JAVA 17.

Permissions
-----------

File system permissions:

- `xdg-pictures`: because this doesn't support portals
- persist: two directories left by Java that we think should be
  persisted.


Other

- X11 only
- Added DRI, albeit not sure it uses it.

Other
-----

Source is from git because submodules are needed.

`lensfun` is installed, directly lifted from Darktable flatpak.

`javahelp2` is needed but not provided with dependencies. Seems that
it's put in the right location.

Install steps are guess from packaging scripts.

Fonts look ugly. Maybe it's just Java.
