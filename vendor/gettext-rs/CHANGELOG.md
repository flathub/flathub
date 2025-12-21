# Changelog

## 0.7.7 - 2025-10-11

### Changed

- Allow gettext-sys 0.26



## 0.7.6 - 2025-10-11

### Changed

- Allow gettext-sys 0.25



## 0.7.5 - 2025-10-11

### Changed

- Document that GNU gettext tools support Rust since 0.24, so `xgettext` can be
    used in place of `xtr` (#139) (Bruno Haible)
- Allow gettext-sys 0.24



## 0.7.4 - 2025-10-10

### Changed

- Allow gettext-sys 0.23



## 0.7.3 - 2025-10-10

### Added

- Support for glibc/libintl shipped with GNU/Hurd (Pino Toscano)



## 0.7.2 - 2024-10-10

### Changed

- Allow gettext-sys 0.22



## 0.7.1 - 2024-08-30

### Fixed

- `TextDomain` not following symlinks (Julian Schmidhuber)



## 0.7.0 - 2021-04-25

### Changed
- If `XDG_DATA_DIRS` environment variable is not set, or is empty, `TextDomain`
    builder defaults to "/usr/local/share/:/usr/share/" instead of current
    directory. This is in line with XDG base directory specification. (Alexander
    Batischev)



## 0.6.0 - 2021-03-03

### Added
- Documentation on when functions will panic (Alexander Batischev)
- Functions to query gettext configuration. These are wrappers over
    `textdomain(NULL)`, `bindtextdomain(domain, NULL)`, and
    `bind_textdomain_codeset(domain, NULL)` (Alexander Batischev)
- `impl Error` for `TextDomainError`, which makes it easier to use with `?`
    operator (Alexander Batischev)

### Changed
- **Users are required to configure gettext for UTF-8, either explicitly with
    `bind_textdomain_codeset()`, or implicitly with `setlocale()` if they understand
    the consequences.** Failure to do this might lead to panics and/or garbled
    data (Alexander Batischev)
- Functions now require more specific types, like `Into<String>` and
    `Into<PathBuf>` (Alexander Batischev)
- Functions that take multiple strings no longer require all strings to be of
    exact same type (i.e. you can mix `String` and `&str`) (Alexander Batischev)
- Functions that configure gettext (`textdomain()`, `bindtextdomain()`,
    `bind_textdomain_codeset()`) now return a `Result` to indicate failures
    (Alexander Batischev)
- Macros now behave like `format!(gettext(…), …)`, which is more useful than
    previous behaviour of `gettext(format!(…, …))` (Rasmus Thomsen)
- On Windows, `TextDomain` now uses `wbindtextdomain` (Alexander Batischev)
- Bump `gettext-sys` dependency to 0.21 (Alexander Batischev)

### Fixed
- `CString`s being dropped while their pointers are in use (Alexander Batischev)

### Removed
- `Default` instance for `TextDomain`. Default-constructed instance was useless
    since it would panic on `init()` as `name` is not set (Alexander Batischev)



## 0.5.0 - 2020-09-01

### Changed
- Bump `locale_config` dependency to 0.3 (Josh Stone)



## 0.4.4 - 2019-09-22

### Added
- Macros that do `gettext(format!(…, …))` (Rasmus Thomsen)



## 0.4.3 - 2019-08-16

### Added
- `Clone` and `Copy` impls for `LocaleCategory` (Cecile Tonglet)



## 0.4.2 - 2019-07-26

### Changed
- Bump `gettext-sys` dependency to 0.19.9 (Konstantin V. Salikhov)



## 0.4.1 - 2018-08-17

### Added
- `pgettext` and `npgettext` functions that support contexts (Daniel García
    Moreno)



## 0.4.0 - 2018-05-23

### Added
- `TextDomain` builder that abstracts over `setlocale`, `textdomain`,
    `bindtextdomain`, and `bind_textdomain_codeset` (François Laignel)

### Changed
- `setlocale` now returns `Option<String>` to indicate that the requested locale
    couldn't be installed (Brian Olsen)

### Removed
- Raw FFI bindings are now in gettext-sys crate (Brian Olsen)



## 0.3.0 - 2016-02-04

### Changed
- Main module renamed from `gettext_rs` to `gettextrs` (Konstantin V. Salikhov)



## 0.2.0 - 2016-02-03

### Added
- Support for plurals (`d*gettext`, `n*gettext`) (Konstantin V. Salikhov)
- Bindings for `bind_textdomain_codeset` (Konstantin V. Salikhov)



## 0.1.0 - 2016-02-02

Initial release (Konstantin V. Salikhov, Faizaan).
