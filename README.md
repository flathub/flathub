# Rukn

## Your Simple and Beautiful Adhan App

Rukn is a simple yet powerful Adhan app designed to be a companion in your daily life. It offers a
clean, beautiful interface free from advertisements and tracking, delivering precise prayer alerts
for any city worldwide. The app includes dual calendar support (Gregorian & Hijri), multiple
calculation methods, and the ability to view prayer schedules for any date - past or future. A range
of thoughtful settings allows you to tailor the experience to your personal needs.

**Features:**

- Prayer times for nearly any city around the globe
- Qibla direction for current location
- Countdown to the next prayer, also visible in tooltips
- View prayer times for past or future dates
- Support for various global calculation methods
- Flexible time format options
- Fine-tune prayer times and Hijri date offsets
- Option to assign a unique and custom adhan for each prayer
- Individual control to enable, mute, or adjust adhan volumes
- Lightweight design with no advertisements or tracking
- Automatic switching between Dark and Light themes
- Customizable UI visual effects
- Dynamic wallpapers that reflect the time of prayer
- Language choices from Arabic and Bangla, with complete RTL layout support
- Convenient system-tray support

This app continues the open-source effort of now-closed-source [Munadi](https://munadi.org)

## Screenshots

### Version 26.11

<div style="overflow-x: auto; white-space: nowrap;">

| ![Main Page - Fajr](https://gitlab.com/lotconn/Rukn/raw/master/screenshots/2611/fajr.png) | ![Main Page - Sunrise](https://gitlab.com/lotconn/Rukn/raw/master/screenshots/2611/sunrise.png) | ![Main Page - Dhuhr](https://gitlab.com/lotconn/Rukn/raw/master/screenshots/2611/dhuhr.png) | ![Main Page - Asr](https://gitlab.com/lotconn/Rukn/raw/master/screenshots/2611/asr.png) | ![Main Page - Maghrib](https://gitlab.com/lotconn/Rukn/raw/master/screenshots/2611/maghrib.png) | ![Main Page - Isha](https://gitlab.com/lotconn/Rukn/raw/master/screenshots/2611/isha.png) |
| :---------------------------------------------------------------------------------------: | :---------------------------------------------------------------------------------------------: | :-----------------------------------------------------------------------------------------: | :-------------------------------------------------------------------------------------: | :---------------------------------------------------------------------------------------------: | :---------------------------------------------------------------------------------------: |
|                                           Fajr                                            |                                             Sunrise                                             |                                            Dhuhr                                            |                                           Asr                                           |                                             Maghrib                                             |                                           Isha                                            |

| ![Material Light Theme](https://gitlab.com/lotconn/Rukn/raw/master/screenshots/2611/material-light.png) | ![Material Dark Theme](https://gitlab.com/lotconn/Rukn/raw/master/screenshots/2611/material-dark.png) | ![Settings Page - Light](https://gitlab.com/lotconn/Rukn/raw/master/screenshots/2611/settings-1.png) | ![Settings Page - Dark](https://gitlab.com/lotconn/Rukn/raw/master/screenshots/2611/settings-2.png) |
| :-----------------------------------------------------------------------------------------------------: | :---------------------------------------------------------------------------------------------------: | :--------------------------------------------------------------------------------------------------: | :-------------------------------------------------------------------------------------------------: |
|                                             Material Light                                              |                                             Material Dark                                             |                                         Settings Page (Dark)                                         |                                        Settings Page (Light)                                        |

| ![Tray Menu](https://gitlab.com/lotconn/Rukn/raw/master/screenshots/2611/tray-menu-light.png) | ![Tray Tooltip](https://gitlab.com/lotconn/Rukn/raw/master/screenshots/2611/tray-tooltip.png) |
| :-------------------------------------------------------------------------------------------: | :-------------------------------------------------------------------------------------------: |
|                                           Tray Menu                                           |                                            Tooltip                                            |

</div>

> [!Note] This repository holds only the flathub manifest and flatpak related contents. For full
> project details, go to [project home](https://gitlab.com/lotconn/rukn).

## Status (Flatpak)

[![Build pipeline](https://github.com/flathub-infra/vorarbeiter/actions/workflows/build.yml/badge.svg)](https://github.com/flathub-infra/vorarbeiter/actions/workflows/build.yml)

Supported Architectures:

- x86_64
- aarch64

## Installing

<div align="center">
  <a href='https://flathub.org/apps/details/org.lotconn.Rukn'>
    <img height='96' alt='Download on Flathub' src='https://flathub.org/assets/badges/flathub-badge-en.png'/>
  </a>
</div>

### Other Sources

#### Rukn Self-Extracting Installer

[![Latest Release](https://gitlab.com/lotconn/rukn/-/badges/release.svg)](https://gitlab.com/lotconn/rukn/-/releases)

No dependencies required - just download & install:

```bash
# Replace VER with the file's version, e.g. Rukn-26.11.run
sh Rukn-<VER>.run -q
```

Alternatively:

```bash
sh ./Rukn-<VER>.run
```

and follow instructions.

## Contributing and Building from Source

See [CONTRIBUTING](CONTRIBUTING.md) for details about building the app and development notes

## Bug Reports

First of all, see if the issue you're facing already exists on the board. If not, follow these
workflow before reporting bugs:

1. Run `flatpak update --appstream` and `flatpak update -y`
2. Remove old configs and caches:

```bash
mv ~/.var/app/org.lotconn.Rukn{,.bak}
```

3. Run from terminal:

```bash
flatpak run \
  --command=rukn \
  --env=QT_LOGGING_RULES='*.debug=true;qt.*.info=true;kf.*.debug=false;qt.qml.import=true' \
  org.lotconn.Rukn > rukn.log 2>&1
```

and try reproducing the issues.

If issues persist, [open an issue](https://gitlab.com/lotconn/rukn/-/work_items) with `rukn.log`,
the install logs you retained earlier and the one in the CWD when installing. Explain as detail as
possible with your current app version and relevant distro details.

## License

This project is licensed under the AGPL-3.0-or-later. Licenses apply to individual tools used in
this project. See the [LICENSE](https://gitlab.com/lotconn/Rukn/blob/master/LICENSE) file for
details, and
[Acknowledgements](https://gitlab.com/lotconn/Rukn/blob/master/README.md#acknowledgements) for
licenses of used libraries.

### Contributors

- **Project Author and Flathub Maintainer:** Hasan Esa [@HasanEsa](https://github.com/HasanEsa)

---

- **Homepage**: [lotconn.org](https://lotconn.org)
- **Bug Tracker**: [Issues](https://gitlab.com/lotconn/rukn/-/issues) |
  [Flathub Issues](https://github.com/flathub/org.lotconn.Rukn/issues)
- **Source Code**: [Source](https://gitlab.com/lotconn/rukn) |
  [Flathub](https://github.com/flathub/org.lotconn.Rukn)

---

_For historical context, see the [CHANGELOG](CHANGELOG.md)._
