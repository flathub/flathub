# tev
High dynamic range (HDR) image viewer for people who care about colors.

This repo hosts the flatpak version of [tev](https://github.com/tom94/tev),
available at [Flathub](https://flathub.org/en/apps/io.github.tom94.tev).

## Network Protocol
tev can be controlled remotely over the network using a simple TCP-based protocol.

To use this, you need to change the sandbox permissions (for example with [Flatseal](https://flathub.org/en/apps/com.github.tchx84.Flatseal)
or with `flatpak override --user --share=network io.github.tom94.tev`) to give tev
access to the network.
