# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Types of changes:

- `Added` for new features.
- `Changed` for changes in existing functionality.
- `Deprecated` for soon-to-be removed features.
- `Removed` for now removed features.
- `Fixed` for any bug fixes.
- `Security` in case of vulnerabilities.

## [Unreleased]

## [0.8.3]

### Added

- rustfmt.toml to enforce formatting

### Changed

- Update README/docs to clarify MacOS Usage (#60 thank you! @ILikeTeaALot)

### Fixed

- Standardized line ending behavior in Git
- Implement error for all error types (#63 thank you! @nick42d)
- Exclude dbus on Android (#59 thank you! @will3942)

## [0.8.2]

### Fixed

- Failing iOS import (#57 Thank you @will3942!)

## [0.8.1]

### Fixed

- Android platform module error
- Linux CI failing due to missing dependency

## [0.8.0]

### Added

- iOS support (#55 Thank you @XMLHexagram !)
- Error handling for zbus, and unify dbus/zbus Error type (#52 Thank you @taoky !)

### Fixed

- Clippy lints (#51 Thank you @LucasFA !)

## [0.7.3]

### Added

- Documentation for `MediaControlEvent::SetVolume` #47

## [0.7.2]

### Changed

- Bumped MSRV to 1.67

## [0.7.1]

### Added

- MSRV in Cargo.toml and rust-toolchain (#46)
- CHANGELOG.md (#45)

### Changed

- Lowered MSRV back to 1.60 from 1.74 (#46)
- Updated CI to support MSRV (#46)
- Updated CI dependencies

## [0.7.0]

### Added

- Implemented volume control on Linux (#42)

### Changed

- Refactored D-Bus module (#42)
