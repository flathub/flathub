# BloomFlatpak
flatpak configuration files for BloomDesktop

This directory holds flatpak packaging specification files and tools for Bloom
Desktop.

## Description of Bloom Desktop

Bloom Desktop is an application that dramatically "lowers the bar" for
language communities who want books in their own languages. Bloom delivers
a low-training, high-output system where mother tongue speakers and their
advocates work together to foster both community authorship and access to
external material in the vernacular.

## Building

### Dependencies

Flathub repo, Bloom flatpak manifest, tools:

```bash
sudo add-apt-repository ppa:flatpak/stable
sudo apt install flatpak-builder flatpak
flatpak --user remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak --user install flathub org.gnome.Sdk//41
flatpak --user install flathub org.gnome.Platform//41
flatpak --user install flathub org.freedesktop.Sdk.Extension.mono6//21.08
flatpak --user install flathub org.freedesktop.Sdk.Extension.node16//21.08
flatpak --user install flathub org.freedesktop.Platform.ffmpeg-full//21.08
flatpak update
```

#### Optional dependencies

Install xonsh to run some dependency-url-generating scripts.
```bash
sudo apt install xonsh
```

### Build

Build and install the flatpak package on the local machine:

```bash
set -xueo pipefail
XDG_CACHE_HOME="${XDG_CACHE_HOME-"${HOME}/.cache"}"
LOG_DIR="${XDG_CACHE_HOME}/flatpak-build-logs"
mkdir -p "${LOG_DIR}"
# Duplicate output to a log file
exec &> >(tee "${LOG_DIR}/flatpak-build-$(date +"%F-%H%M%S").log")

# Optionally delete the build dirs from last time. But then --keep-build-dirs,
# not --delete-build-dirs so that the build dirs from this time are left in
# place for optional inspection.
#rm -rf .flatpak-builder/build
mkdir -p ../flatpak/local-repo
flatpak-builder --user --repo=../flatpak/local-repo --keep-build-dirs --force-clean ./build-dir org.sil.Bloom.yml "$@"
flatpak --user remote-add --no-gpg-verify --if-not-exists local-repo ../flatpak/local-repo
# If a Bloom flatpak is already installed, such as from a downloaded
# .flatpak file, remove it so we don't fail to install what we just built.
flatpak remove --assumeyes org.sil.Bloom org.sil.Bloom.Debug org.sil.Bloom.Locale ||
    echo "flatpak packages were not already installed."
flatpak --user install --or-update --noninteractive local-repo org.sil.Bloom org.sil.Bloom.Debug
```

Note that your first build will take a long time to download and build all
dependencies. Subsequent builds benefit from caching.

## Testing

Run Bloom in the flatpak package:

```bash
flatpak run org.sil.Bloom
```

Open a shell inside the Bloom flatpak instead of running Bloom:

```bash
flatpak run --command=bash org.sil.Bloom
```

## Validate appdata

```bash
flatpak install flathub org.freedesktop.appstream-glib
flatpak run org.freedesktop.appstream-glib validate extra/org.sil.Bloom.metainfo.xml
```

## Clean up

You can safely delete the following directories, although it will make the next
package-build take longer (possibly 3 hours longer!):

```
.flatpak-builder
build-dir
../flatpak/local-repo
~/.cache/flatpak-build-logs
```

## Search github for flatpak build using component XXX
```
https://github.com/search?q=org%3Aflathub+XXX&type=code
```
