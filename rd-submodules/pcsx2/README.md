# PCSX2 (Flathub)
net.pcsx2.PCSX2

[PCSX2 Official Website](https://pcsx2.org)

## Building
```bash
$> flatpak-builder builddir --force-clean --install-deps-from=flathub net.pcsx2.PCSX2.json
```

## Networking Support
As of PCSX2 1.7, DEV9 support has been merged into the main codebase and can be used with some caveats.

#### Permissions
Since Flatpak does not contain permissions to write to the network stack via libpcap at the user level, PCSX2 will likely need to be run as `root`
and will need the `--share=network` permissions. (Note: Running apps like this under root is not a good idea.)

If networking still does not work, or adapters do not show up, you will need to grant cap permissions to PCSX2
```bash
$> setcap cap_net_raw,cap_net_admin=eip /var/lib/flatpak/app/net.pcsx2.PCSX2/current/active/files/bin/PCSX2
```

#### Audio Support
PulseAudio will need to be ran in multi-user mode to have sound support if running under root or some other user.
```bash
$> flatpak run --system --socket=pulseaudio net.pcsx2.PCSX2`
```
