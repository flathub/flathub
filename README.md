## Upstream
https://invent.kde.org/plasma/discover


## Patches

stolen from [gnome builder](https://github.com/flathub/org.gnome.Builder) and [bazaar](https://github.com/flathub/io.github.kolunmi.Bazaar)

# broken stuff
- launch button doesn't do anything
- ~- no --user flatpaks~
- can't install .flatpak bundle files (needs permission to the path, inside the sandbox flatpak install --user /run/user/1000/doc/xxx/ works)
- ~FWUPD backend~
- KNS backend
- kcm_updates
- can't launch kcm_flatpak
- [extra flatpak installations](https://docs.flatpak.org/en/latest/tips-and-tricks.html#adding-a-custom-installation) even if you give permission to host-etc and the path you are installing to
- kuserfeedback [See](https://github.com/renner0e/flathub/commit/1c25f82a28e2dcf5fa00a7c9eda23f9513e4ddb5)
- WebFlow stuff authenticating to a flatpak repository and handling that authentication via browser (needs qtwebengine)

# Need to be investigated
- fiwrmware upgrades with fwupd backend
- PackageKit

# confirmed working
- right clicking in kickoff to uninstall

https://github.com/user-attachments/assets/fd0bf708-b011-44ff-9e3f-b49febf9f9fb

