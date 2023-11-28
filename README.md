# Azure Storage Explorer Flatpak

## Installation
You'll need `flatpak-builder` to build the image.

Pull the git repo and run `flatpak-builder --user --install --force-clean build-dir com.microsoft.AzureStorageExplorer.yaml` for test installation.

## Support
### KDE
On KDE, you may need either KWallet with KDE Frameworks version >5.97 or a different secrets manager. Because of Kwallet's implementation of `org.freedesktop.secrets` [isn't supported by keytar](https://github.com/atom/node-keytar/issues/74) you may run into issues with KWallet. An easy solution is using `gnome-keyring` or KeepassXC secret service. For Kubuntu 22.04 LTS, you need to enable backports.