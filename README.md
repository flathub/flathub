# OpenCloud Desktop

OpenCloud Desktop is the official desktop client for OpenCloud, a modern file synchronization and sharing solution. This package is using 1:1 the sources of this amazing project to provide a flathub package for it.

## Features
- File synchronization with OpenCloud servers
- Secure file sharing and collaboration
- End-to-end encryption support
- Integrated with your desktop environment

## License

This Flatpak app is based on "The OpenCloud Desktop application", originally developed by the OpenCloud community.
Source code available at: https://github.com/opencloud-eu/desktop .
Licensed under GPLv2.

## Build Instructions
```bash
flatpak-builder --repo=repo --force-clean builddir com.handtrixxx.OpenCloud.yml
flatpak install --user repo com.handtrixxx.OpenCloud
```
