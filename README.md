<p align="center">
<img width="154" height="154" alt="logo" src="https://github.com/user-attachments/assets/42b73dbf-778c-45ff-9361-22a52988f1b3" />
</p>

**sshPilot** is a user-friendly, modern and lightweight SSH connection manager for Linux and macOS. It's a free alternative to Putty and Termius.

<p align="center">
<img width="1260" height="833" alt="Main window" src="https://github.com/user-attachments/assets/743bb1fb-22de-4537-ba91-775cea48d57a" />

<img width="722" height="822" alt="Connection settings" src="https://github.com/user-attachments/assets/55fad9a6-9d4d-4c15-bfac-8c19c6df15c5" />
</p>


## Features

- Tabbed interface
- Full support for Local, Remote and Dynamic port forwarding 
- Intuitive, minimal UI with keyboard navigation and shortcuts
- SCP support for quicly uploading a file to remote server
- Keypair generation and copying to remote servers (ssh-copy-id)
- Support for running remote and local commands upon login
- Secure storage for credentials using libsecret
- Privacy toggle to show/hide ip addresses/hostnames in the main window
- Light/Dark interface themes
- Customizable terminal font and color schemes
- Load/save standard .ssh/config entries
- Free software (GPL v3 license)

## Installation 

### Linux
The app is currently distributed as deb and rpm packages (see releases) and can be installed on recent versions of Debian (testing/unstable), Ubuntu and Fedora. Debian bookworm is not supported due to older libadwaita version. 
A Flatpak is also planned for future.

### macOS

(WIP) On the [Mac branch](https://github.com/mfat/sshpilot/tree/mac) there are [instructions](https://github.com/mfat/sshpilot/blob/mac/INSTALL-macos.md) for running sshPilot on macOS



## Download

Latest release can be downloaded from here: https://github.com/mfat/sshpilot/releases/

You can also run the app from source. Install the modules listed in requirements.txt and a fairly recent version of GNOME and it should run.

`
python3 run.py
`




Runtime dependencies
--------------------

Install system GTK/libadwaita/VTE GI bindings (do not use pip for these).

Debian/Ubuntu (minimum versions)

```
sudo apt update
sudo apt install \
  python3 python3-gi python3-gi-cairo \
  libgtk-4-1 (>= 4.6) gir1.2-gtk-4.0 (>= 4.6) \
  libadwaita-1-0 (>= 1.4) gir1.2-adw-1 (>= 1.4) \
  libvte-2.91-gtk4-0 (>= 0.70) gir1.2-vte-3.91 (>= 0.70) \
  python3-paramiko python3-cryptography python3-secretstorage python3-matplotlib sshpass
```

Fedora / RHEL / CentOS


```
sudo dnf install \
  python3 python3-gobject \
  gtk4 libadwaita \
  vte291-gtk4 \
  libsecret \
  python3-paramiko python3-cryptography python3-secretstorage python3-matplotlib sshpass
```

Run from source


```
python3 run.py
```



## Keyboard/mouse navigation and shortcuts

sshPilot is easy to navigate with keyboard. When the app starts up, just press enter to connect to the first host in the list. You can do the same thing by double-clicking the host.
Press ctrl+L to quickly switch between hosts, close tabs with ctrl+F4 and switch tabs with alt+right/left arrow.
If you have multiple connections to a single host, doble-clicking the host will cycle through all its open tabs.