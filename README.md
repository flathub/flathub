# org.quetoo.Quetoo

## Prerequisites

```bash
flatpak install flathub org.freedesktop.Sdk
flatpak install flathub org.freedesktop.Platform
```

## Build

```bash
cd ~/Coding
git clone https://github.com/maci0/org.quetoo.Quetoo.git
flatpak-builder --force-clean --repo=quetoo-repo build org.quetoo.Quetoo/org.quetoo.Quetoo.json
```
## Install & Test

```bash
flatpak --user remote-add --no-gpg-verify --if-not-exists quetoo-repo quetoo-repo
flatpak --user install quetoo-repo org.quetoo.Quetoo
flatpak run org.quetoo.Quetoo
```
