# notes
Here are my notes that I've been keeping in order to test this build locally. Sadly since I'm on tauri v1 it means we have to compile webkitgtk-4.0 from source which takes a LOONG time.

It's not even feasable to allow me to test via a github action so I'm using my gaming laptop+WSL.

Build/run the app via [WSLG](https://github.com/microsoft/wslg).
```
./build.sh && ./test.sh
```

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

It also still took me around an hour to compile for the first time 😂. Then after that with the `--ccache` flag it takes around 5-10 mins.

### Blocker
It wont run currently:
```
root@gamer:~/flathub# flatpak remotes
Name             Options
flathub          system
flathub          user
overlayed-origin user,no-enumerate,no-gpg-verify
root@gamer:~/flathub# flatpak run dev.overlayed.Overlayed
root@gamer:~/flathub# flatpak --verbose run dev.overlayed.Overlayed
F: No installations directory in /etc/flatpak/installations.d. Skipping
F: Opening system flatpak installation at path /var/lib/flatpak
F: Opening user flatpak installation at path /root/.local/share/flatpak
F: Opening user flatpak installation at path /root/.local/share/flatpak
F: Skipping parental controls check for app/dev.overlayed.Overlayed/x86_64/master since parental controls are disabled globally
F: Opening user flatpak installation at path /root/.local/share/flatpak
F: /root/.local/share/flatpak/runtime/org.gnome.Platform/x86_64/46/cbd6597a7a131f6ddfe5db99c3f761dc8921b81b7b05a5e210924e6b5b5f53f4/files/lib32 does not exist
F: Cleaning up unused container id 1847038982
F: Cleaning up per-app-ID state for dev.overlayed.Overlayed
F: Allocated instance id 3815818615
F: Add defaults in dir /dev/overlayed/Overlayed/
F: Add locks in dir /dev/overlayed/Overlayed/
F: Allowing dri access
F: Allowing homedir access
F: Allowing wayland access
F: Allowing pulseaudio access
F: Running 'bwrap --args 36 xdg-dbus-proxy --args=40'
F: Running 'bwrap --args 37 overlayed'
```

### TODO:

- fix the icon
- fix the desktop icon