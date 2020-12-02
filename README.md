## typora

### Here follows some commands I use to test and prefer to keep together with the repo for quick copy and paste:
```bash
sudo zypper install flatpak flatpak-builder
sudo flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
sudo flatpak -y --system install flathub org.freedesktop.Platform//20.08 org.freedesktop.Sdk//20.08

# BUILD
flatpak-builder --force-clean --repo=test-repo-typora build-dir io.typora.editor.json

# TEST
flatpak-builder --run build-dir io.typora.editor.json /app/start.sh
flatpak-builder --run build-dir io.typora.editor.json /bin/bash

# INSTALL
sudo flatpak remote-add --no-gpg-verify test-repo-typora test-repo-typora
flatpak -y remove io.typora.editor
flatpak -y --system install test-repo-typora io.typora.editor
flatpak run io.typora.editor
flatpak run --command=/bin/bash io.typora.editor

# EXPORT
flatpak build-bundle test-repo-typora typora-0.9.96.flatpak io.typora.editor

# IMPORT
sudo flatpak -y remove io.typora.editor
sudo flatpak -y install typora-0.9.96.flatpak

```
