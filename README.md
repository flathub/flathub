## typora

### Here follows some commands I use to test and prefer to keep together with the repo for quick copy and paste:
```bash
sudo zypper install flatpak flatpak-builder
sudo flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
sudo flatpak -y --system install flathub org.freedesktop.Platform//20.08 org.freedesktop.Sdk//20.08

# BUILD
flatpak-builder --force-clean --repo=test-repo-typora build-dir io.typora.flatpak.json

# TEST
flatpak-builder --run build-dir io.typora.flatpak.json /app/start.sh
flatpak-builder --run build-dir io.typora.flatpak.json /bin/bash

# INSTALL
sudo flatpak remote-add --no-gpg-verify test-repo-typora test-repo-typora
flatpak -y remove io.typora.flatpak
flatpak -y --system install test-repo-typora io.typora.flatpak
flatpak run io.typora.flatpak
flatpak run --command=/bin/bash io.typora.flatpak

# EXPORT
flatpak build-bundle test-repo-typora typora-0.9.96.flatpak io.typora.flatpak

# IMPORT
sudo flatpak -y remove io.typora.flatpak
sudo flatpak -y install typora-0.9.96.flatpak

```
