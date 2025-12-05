# Work Log Desktop - Flathub Package

Official Flatpak package for Work Log Desktop - a Qt/KDE application for tracking work sessions with hierarchical time organization.

## About

Work Log Desktop helps you track and organize your work sessions by day, week, month, and year. Features include:

- **Hierarchical Navigation**: Browse sessions by year → month → week → day
- **Comprehensive Session Tracking**: Record time, descriptions, notes, and planning for each work session
- **Tag System**: Organize and categorize sessions with custom tags
- **Statistics**: View daily, weekly, and monthly hour totals at a glance
- **Calendar Integration**: Easy date selection with calendar picker
- **Local Storage**: All data stored locally in SQLite database

## Installation

Once approved on Flathub, install with:

```bash
flatpak install flathub work.worklog.worklog
```

Run with:

```bash
flatpak run work.worklog.worklog
```

## Build Configuration

This Flatpak package is built with the following configuration:

- **Runtime**: org.kde.Platform 5.15-25.08
- **Sync Feature**: Disabled (the source repository contains optional cloud sync functionality, but it is explicitly disabled in the Flatpak build via `-DENABLE_SYNC=OFF`)
- **Network Access**: Not requested (fully offline application)
- **Permissions**: Minimal sandbox permissions (display, GPU, local data directories only)

### Why is sync disabled?

The source repository at https://github.com/macsplit/Work.Log contains optional cloud sync functionality for users who build from source. However, in alignment with Flatpak/Flathub philosophy of minimal permissions and no mandatory external dependencies, the Flatpak build:

- Uses conditional compilation (`-DENABLE_SYNC=OFF`)
- Does not compile sync-related code
- Requests no network permissions
- Provides a completely standalone, offline-capable experience

See [FLATPAK.md](https://github.com/macsplit/Work.Log/blob/main/WorkLog.Desktop/FLATPAK.md) in the source repository for detailed technical information.

## Data Storage

The application stores all data locally within the Flatpak sandbox:

- **Database**: `~/.var/app/work.worklog.worklog/data/WorkLog/worklog.db`
- **Configuration**: `~/.var/app/work.worklog.worklog/config/WorkLog/`

Your data is completely private and stored only on your local machine.

## Source Code

- **Main Repository**: https://github.com/macsplit/Work.Log
- **Desktop App Source**: https://github.com/macsplit/Work.Log/tree/main/WorkLog.Desktop
- **License**: GPL-3.0-or-later

## Building Locally

To test the Flatpak build locally:

```bash
# Install dependencies
flatpak install flathub org.kde.Platform//5.15-25.08 org.kde.Sdk//5.15-25.08

# Build
flatpak-builder --user --install --force-clean build-dir work.worklog.worklog.json

# Run
flatpak run work.worklog.worklog
```

## Screenshots

![Work Log Desktop Main Window](https://raw.githubusercontent.com/macsplit/Work.Log/main/screenshots/desktop-main.png)

*Main window showing hierarchical session navigation, session list, and details panel*

## Links

- **Homepage**: https://github.com/macsplit/Work.Log
- **Bug Reports**: https://github.com/macsplit/Work.Log/issues
- **Developer**: Lee Hanken (https://github.com/macsplit)

## Technical Details

- **App ID**: work.worklog.worklog
- **Build System**: CMake + Ninja
- **Frameworks**: Qt 5.15, KDE Frameworks 5 (Kirigami, i18n, CoreAddons)
- **Database**: SQLite 3
- **UI Framework**: QML with Kirigami components

## Support

For issues, questions, or feature requests, please visit:
https://github.com/macsplit/Work.Log/issues
