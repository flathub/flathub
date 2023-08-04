This SDL2 module is intended for use with flatpaks that rely on SDL2. 

If your app or game wants to use a newer version of SDL2, or default to Wayland, you can use this. Set the following permissions for using Wayland:

```
socket=wayland
socket=fallback-x11
share=ipc
```

This SDL2 module contains two manifests; one containing SDL2 without libdecor, and the other with.

If your application utilizes SDL2 but doesn't work on Wayland, even with libdecor, you might opt for the `-no-libdecor` manifest, and disable Wayland support.

Otherwise, flatpak maintainers can use the `-with-libdecor` manifest.
