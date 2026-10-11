# Ledger Wallet (Ledger Live) – unofficial Flatpak

Repackages the official AppImage (`com.ledger.LedgerLive`) on top of `org.electronjs.Electron2.BaseApp` (zypak).
The AppImage is not redistributed: it is downloaded on the user's machine at install time (`extra-data`) and extracted by `data/apply_extra.sh`.

## Build / install
    flatpak install flathub org.flatpak.Builder org.electronjs.Electron2.BaseApp//26.08
    flatpak run org.flatpak.Builder --force-clean --disable-rofiles-fuse --repo=repo build com.ledger.LedgerLive.yml
    flatpak remote-add --user --no-gpg-verify ledger-local repo
    flatpak install --user ledger-local com.ledger.LedgerLive
    flatpak run com.ledger.LedgerLive

## Host requirement: udev rules
Flatpak cannot install them. On Arch: `pacman -S ledger-udev`
(otherwise: https://github.com/LedgerHQ/udev-rules).

## Updating
Change `url`, `sha256` and `size` of the `extra-data` source in the manifest (or use flatpak-external-data-checker).
