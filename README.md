# DevPod flathub repo

This repo contains the flatpak manifest used to build and distribute devpod via flathub.

The manifest sh.loft.Devpod.yaml uses the release pipeline of the [main devpod repo](https://github.com/loft-sh/devpod) to create a .deb package, the flatpak build commands defined in sh.loft.devpod.yaml then extracts the Desktop application, CLI binary, .desktop and metadata files as described in https://v2.tauri.app/distribute/flatpak/.

Please note: devpod requires privileged access to host resources such as /home, unix sockets like SSH_AUTH_SOCK etc. to operate. It uses the shim devpod-wrapper in order for the CLI to run on the host while the desktop application is running in the sandbox.