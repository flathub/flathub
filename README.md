## Chromium Flatpak

### Live shell

`build_devel_flatpak.py` can be used to access a live shell inside the build environment
for performing a local from-source build for development purposes.

### Build arguments to pass to gn

```
# Not supported
use_udev = false
# Not required but makes builds faster.
use_lld = true
# NaCL hasn't been tested and is being removed from Linux builds.
enable_nacl = false
# Unrelated to Flatpak but helps speed up builds.
blink_symbol_level = 0
# Outdated
use_gnome_keyring = false
# Not supported
use_sysroot = false
```
