# DevPod flathub repo

This repo contains the flatpak manifest used to build and distribute devpod via flathub.

The manifest sh.loft.Devpod.yaml uses the release pipeline of the [main devpod repo](https://github.com/loft-sh/devpod) to create a single file flatpak bundle using the .deb package, as described in https://v2.tauri.app/distribute/flatpak/.

Please note: devpod requires privileged access to host resources such as /home, unix sockets like SSH_AUTH_SOCK etc. to operate.