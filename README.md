# OpenTabletDriver Flatpak Package

Welcome to the unofficial Flatpak package for OpenTabletDriver! This Flatpak package, `com.opentabletdriver.OpenTabletDriver`, is maintained by an independent developer passionate about making OpenTabletDriver more accessible and easier to install on Linux distributions. While this is not an official package from the OpenTabletDriver project team, it aims to deliver the same great user experience and functionality.

## About OpenTabletDriver

OpenTabletDriver is an open-source, cross-platform tablet driver offering high compatibility and performance for a wide range of graphics tablets. It features an easily configurable graphical user interface, making it possible for users to fine-tune their tablets to match their exact needs. OpenTabletDriver supports absolute and relative cursor positioning, pen bindings, and even custom plugins for enhanced functionality.

## Installation

To install this Flatpak package, you will need to have Flatpak installed on your system. Most modern Linux distributions either come with Flatpak pre-installed or provide easy methods to set it up. Please refer to the [Flatpak official documentation](https://flatpak.org/setup/) for instructions specific to your distribution.

Once Flatpak is installed, you can install the `com.opentabletdriver.OpenTabletDriver` package using the following command in your terminal:

```bash
flatpak install flathub com.opentabletdriver.OpenTabletDriver
```

## Running OpenTabletDriver

After installation, OpenTabletDriver can be launched from your application menu or via the terminal with the following command:

```bash
flatpak run com.opentabletdriver.OpenTabletDriver
```

This command starts the OpenTabletDriver daemon and, if you choose to use it, the GUI for configuring your tablet settings. Remember, the daemon must be running for tablet functionality, but the GUI is optional and used only for configuration purposes.
