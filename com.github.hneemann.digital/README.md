## Digital Logic Simulator ##

This is a Flatpak build of https://github.com/hneemann/Digital, which is a Digital Logic Simulator.

## Building it

aarch64:
```
flatpak install org.freedesktop.Sdk/aarch64/23.08
flatpak install org.freedesktop.Platform/aarch64/23.08
flatpak install org.freedesktop.Sdk.Extension.openjdk21/aarch64/23.08
./build.sh aarch64
```

for building a x86_64 version, just replace aarch64 with x86_64 in the above commands.

## Runtime configuration ##

At least to me, it seems that the OpenJDK has some issues in picking up the correct font
for the UI. As a workaround, i added some configuration options, which are picked up at runtime.

To use it, modify the configuration file $XDG_CONFIG_HOME/digital-options.txt and specify
the default font there.

