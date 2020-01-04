# org.firestormviewer.FirestormViewer

**Unofficial** Flatpak wrapper for Firestorm (https://www.firestormviewer.org/)

### Broken
* ~~Voice~~ *- Fixed*
* ~~Discord~~ *- Fixed*

### Build and Install
Run this line by line.

```shell
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install flathub org.freedesktop.Platform.Compat.i386//19.08 org.freedesktop.Sdk.Extension.toolchain-i386//19.08 org.freedesktop.Sdk.Compat.i386//19.08
git clone --recursive https://github.com/p1u3o/org.firestormviewer.FirestormViewer && cd org.firestormviewer.FirestormViewer
sudo flatpak-builder --system --install --install-deps-from=flathub _build org.firestormviewer.FirestormViewer.json --force-clean
```

Go make a coffee, compiling Wine will take a little while.


#### Notes

* This branch does not build the viewer from source, there is a compile branch but it lacks FMOD.

* Enabling voice for the first time will create a wine prefix to run the Windows Vivox voice plugin.
