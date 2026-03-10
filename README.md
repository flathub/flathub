# Electron Cash Flatpak

Flatpak package for [Electron Cash](https://electroncash.org/), a lightweight SPV Bitcoin Cash wallet.

## Build

```bash
git clone https://github.com/Eggroley/electron-cash-flatpak.git
cd electron-cash-flatpak
flatpak-builder --user --install-deps-from=flathub --force-clean build org.electroncash.ElectronCash.yml
```

## Run

```bash
flatpak run org.electroncash.ElectronCash
```
## Bundle
```bash
flatpak-builder --user --force-clean --repo=repo build org.electroncash.ElectronCash.yml
flatpak build-bundle repo Electron-Cash.flatpak org.electroncash.ElectronCash
```

## Features

- Full SPV wallet for Bitcoin Cash
- CashFusion support with bundled Tor
- Hardware wallet support (Trezor, Ledger, KeepKey, Satochip)
- QR code scanning
- System tray icon support

## Update

1. Get SHA256: `curl -sL <release-url> | sha256sum`
2. Update version and SHA256 in `org.electroncash.ElectronCash.yml`
3. Add release entry to `org.electroncash.ElectronCash.metainfo.xml`
4. Build and test
