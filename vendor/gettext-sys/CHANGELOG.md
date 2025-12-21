# Changelog

## 0.26.0 - 2025-10-11

### Changed

- Bump bundled GNU gettext to 0.26 (Alexander Batischev)



## 0.25.0 - 2025-10-11

### Changed

- Bump bundled GNU gettext to 0.25.1 (Alexander Batischev)



## 0.24.0 - 2025-10-11

### Changed

- Bump bundled GNU gettext to 0.24.2 (Alexander Batischev)



## 0.23.0 - 2025-10-10

Please note that in macOS 15 the `iconv()` call is too broken to be used by GNU
gettext; see the upstream bug: https://savannah.gnu.org/bugs/index.php?66541

### Changed

- Bump bundled GNU gettext to 0.23.2 (Alexander Batischev)



## 0.22.6 - 2025-10-10

### Added

- Support for glibc/libintl shipped with GNU/Hurd (Pino Toscano)

### Changed

- Stop building libgettext binaries that aren't required for gettext-sys. This
    speeds up the compilation of the vendored GNU gettext by an order of
    magnitude (Bruno Haible, Alexander Batischev)

### Fixed

- Build failure on OpenBSD caused by `tar` not supporting `--strip-components`
    and `-J` (#116) (Alfred Morgan, Alexander Batischev)



## 0.22.5 - 2024-10-10

### Changed

- Bump bundled GNU gettext to 0.22.5 (Dennis van der Schagt)



## 0.21.4 - 2024-08-30

### Changed

- One can now link with system-provided gettext on FreeBSD by installing
    `gettext` package or port and enabling `gettext-system` feature (or setting
    `GETTEXT_SYSTEM` environment variable) (Nathan Fisher)

### Fixed

- Build failure with Clang 16 (Alexander Batischev)



## 0.21.3 - 2022-03-16

### Changed
- `wbindtextdomain` is now a Rust function rather than a C symbol. The symbol is
    now named `libintl_wbindtextdomain`, for better compatibility with MinGW.
    This change only affects Windows (Marin)

### Fixed
- Only check for build dependencies when actually building the library (Ignacio
    Casal Quinteiro)



## 0.21.2 - 2021-07-21

### Fixed
- Build failure on some systems which put libraries into "lib64" directory (#73)
    (Alexander Batischev)



## 0.21.1 - 2021-07-16

### Changed
- Dependency on `tempdir` is replaced by dependency on `temp-dir`, which is way
    more lightweight (Amrit Brar)



## 0.21.0 - 2021-03-03

### Added
- A note regarding GNU gettext's LGPL license (Alexander Batischev)
- Checks for build tools required by GNU gettext (Dean Leggo)
- Bindings for `wbindtextdomain` (only available on Windows) (Alexander
    Batischev)
- Build-time dependency on `tempfile` (Alexander Batischev)

### Changed
- Bump bundled GNU gettext to 0.21 (Alexander Batischev)

### Fixed
- Build failure when a path contains spaces (Alexander Batischev)



## 0.19.9 - 2019-07-26

### Added
- Support for Windows+GNU (François Laignel)
- Support for musl libc (Rasmus Thomsen)
- `gettext-system` feature which asks the crate to use gettext that's built into
    libc (if available) (François Laignel)
- Use xz to compress the bundled GNU gettext tarball, to save space (Daniel
    García Moreno)



## 0.19.8 - 2018-05-23

Initial release (Konstantin V. Salikhov, Faizaan).
