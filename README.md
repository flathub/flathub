# notes
Here are my notes that I've been keeping in order to test this build locally. Sadly since I'm on tauri v1 it means we have to compile webkitgtk-4.0 from source which tags a LOONG time.

It's not even feasable to allow me to test via a github action so I'm using my gaming laptop+WSL.

Running the build in WSL:
```
flatpak-builder \
  --repo=repo \
  --disable-rofiles-fuse \
  --install-deps-from=flathub \
  --force-clean \
  --default-branch=master \
  --arch=x86_64 --ccache \
  build-dir/ dev.overlayed.Overlayed.yaml
```

Running the app via [WSLG](https://github.com/microsoft/wslg)

### WSL Gotcha

When first trying to compile it I ran out of memory so I had to add this to my `%USERPROFILE%/.wslconfig`

```
# Settings apply across all Linux distros running on WSL 2
[wsl2]

# Limits VM memory to use no more than 4 GB, this can be set as whole numbers using GB or MB
memory=20GB 

# Sets the VM to use two virtual processors
processors=8

# Sets amount of swap storage space to 8GB, default is 25% of available RAM
swap=8GB
```

It also still took me around an hour to compile 😂.

Latest blocker:
```
Running: mkdir -p /app/{bin,share}
Running: cp -r squashfs-root/usr/share/icons /app/share/icons
Running: install -Dm755 squashfs-root/usr/bin/overlayed /app/bin/overlayed
Running: install -Dm644 squashfs-root/usr/share/applications/overlayed.desktop /app/share/applications/dev.overlayed.Overlayed.desktop
Running: install -Dm644 dev.overlayed.Overlayed.metainfo.xml /app/share/metainfo/dev.overlayed.Overlayed.metainfo.xml
compressing debuginfo in: /root/flathub/build-dir/files/bin/overlayed
processing: /root/flathub/build-dir/files/bin/overlayed
Nothing to do.
stripping /root/flathub/build-dir/files/bin/overlayed to /root/flathub/build-dir/files/lib/debug/bin/overlayed.debug
Committing stage build-overlayed to cache
Cleaning up
Removing files/share/man/man1/unifdefall.1
Removing files/share/man/man1/unifdef.1
Removing files/share/man/man1
Removing files/share/man
Removing files/lib/debug/source/unifdef/unifdef.h
Removing files/lib/debug/source/unifdef/unifdef.c
Removing files/lib/debug/source/unifdef
Removing files/lib/debug/bin/unifdef.debug
Removing files/lib/debug/bin
Removing files/lib/debug
Removing files/lib
Removing files/bin/unifdefall
Removing files/bin/unifdef
Removing files/bin
Renaming dev.overlayed.Overlayed.metainfo.xml to share/appdata/dev.overlayed.Overlayed.appdata.xml
Error: icon Overlayed not found below /root/flathub/build-dir/files/share/icons
```