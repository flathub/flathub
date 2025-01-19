# ControllerBuddy-Flatpak

## Description

This is the repository for the offical Flatpak for [ControllerBuddy](https://controllerbuddy.org), the highly advanced game controller mapping application.

In addition to the application, the Flatpak includes a copy of the [ControllerBuddy-Install-Script](https://github.com/bwRavencl/ControllerBuddy-Install-Script), which is required to automate the following tasks:

- Configuration of [udev](https://www.freedesktop.org/software/systemd/man/udev.html) and [uinput](https://www.kernel.org/doc/html/latest/input/uinput.html) for ControllerBuddy
- Downloading the official [ControllerBuddy-Profiles](https://github.com/bwRavencl/ControllerBuddy-Profiles) to the user's `Documents` folder and keeping them updated

## Shortcuts

Two shortcuts are provided:

- **ControllerBuddy**  
  In order to launch ControllerBuddy, this performs the following tasks:
  1. The ControllerBuddy-Install-Script is run (outside of the sandbox), in a mode that if necessary adds the required udev rules to allow ControllerBuddy access to the uinput device and to Sony DualShock / DualSense HID devices.
  2. If the corresponding folder is still missing in the user's `Documents` folder, the ControllerBuddy-Install-Script will be run again (now inside the sandbox), in a mode that pulls the official ControllerBuddy-Profiles from GitHub.
  3. Finally, ControllerBuddy itself is started with arguments: `-autostart local -tray`
- **Update ControllerBuddy-Profiles**  
  This will run the ControllerBuddy-Install-Script (inside the sandbox) to update the local copy of the ControllerBuddy-Profiles to the latest available version.
