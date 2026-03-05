<!--
Copyright © 2026 <https://github.com/technosf>
SPDX-FileCopyrightText: © 2026 <https://github.com/technosf>

SPDX-License-Identifier: GPL-3.0-or-later
-->
# io.github.tuner_labs.tuner

Flathub Manifest and build information for *io.github.tuner_labs.tuner*

## Builds

[Build Bot](https://builds.flathub.org/status/io.github.tuner_labs.tuner)

### Assets

#### Platform

- org.freedesktop.Platform_ latest version is 25.08

#### Modules

- libgee 0.20.8
- granite 6.2.0

## Process

### Check Metadata

From *[Tuner](https://github.com/technosf/tuner)* source code for the given release tag, verify:

- [ ] *meson.build*  =>  project.version

- [ ] *data/io.github.tuner_labs.tuner.appdata.xml.in* => releases.release version

### Check Build

Before checking in, test the app build and lint:

``` bash
flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --repo=repo builddir io.github.tuner_labs.tuner.yml

flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest io.github.tuner_labs.tuner.yml

flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo
```

Linting results are documented  at <https://docs.flathub.org/linter>

## History

### 2.0.2

- Renaming from *com.github.louis77.tuner* to *io.github.tuner_labs.tuner* per flathub #7468

### 2.0.1

- Much thanks to @yakushabb for streamlining the build
- ~~Renamed app from *com.github.louis77.tuner* to *io.github.tuner_labs.tuner* to reflect the new app name. See [#7468](https://github.com/flathub/flathub/issues/7468)~~
- Added *separate-locales: false* to glob all translations into the same build image
- Rationalized Icons
- [@syakushabb](https://github.com/yakushabb) removed *vala* module as it is now built into *FreeDesktop* 25.08. Module will remain in the source manifest for local builds. See [#40](https://github.com/flathub/com.github.louis77.tuner/pull/40)
- [@syakushabb](https://github.com/yakushabb) removed *libsoup* module as it is now built into *FreeDesktop* 25.08. Module will remain in the source manifest for local builds. See [#43](https://github.com/flathub/com.github.louis77.tuner/pull/43)
