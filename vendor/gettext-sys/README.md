# gettext-sys

Raw FFI bindings for gettext. Please see
[documentation](https://docs.rs/gettext-sys) for details.

## Licensing

On platforms that don't have a native gettext implementation, this crate
compiles GNU gettext, which is licensed under LGPL. This means **you have to
abide by LGPL**. If you don't want or can't do that, there are two ways out:

1. in a GNU environment, enable `gettext-system` feature (see below);
2. dynamically link to GNU gettext library you obtained by some other means,
   like a package manager. See environment variables below.

## Features

- `gettext-system`: if enabled, _asks_ the crate to use the gettext
    implementation that's part of glibc or musl libc. This only works on:

    * Linux with glibc or musl libc;
    * Windows + GNU (e.g. [MSYS2](https://www.msys2.org/)) with
        `gettext-devel` installed e.g. using:

        ```
        pacman --noconfirm -S base-devel mingw-w64-x86_64-gcc libxml2-devel tar
        ```
    * FreeBSD with GNU gettext installed as a package or port;
    * GNU/Hurd with glibc;

    If none of those conditions hold, the crate will proceed to building and
    statically linking its own copy of GNU gettext!

## Environment variables

- `GETTEXT_SYSTEM`: same as enabling `gettext-system` feature (see above).

- `GETTEXT_DIR`: if specified, a directory that will be used to find gettext
    installation. It's expected that under this directory, the _include_ folder
    has header files, the _bin_ folder has gettext binary, and a _lib_ folder
    has the runtime libraries.

- `GETTEXT_LIB_DIR`: if specified, a directory that will be used to find gettext
    libraries. Overrides the _lib_ folder implied by `GETTEXT_DIR` (if specified).

- `GETTEXT_INCLUDE_DIR`: if specified, a directory that will be used to find
    gettext header files. Overrides the _include_ folder implied by
    `GETTEXT_DIR` (if specified).

- `GETTEXT_BIN_DIR`: if specified, a directory that will be used to find gettext
    binaries. Overrides the _bin_ folder implied by `GETTEXT_DIR` (if specified).

- `GETTEXT_STATIC`: if specified, gettext libraries will be statically rather
    than dynamically linked. This only affects `GETTEXT_DIR` and `GETTEXT_*_DIR`
    scenarios; the default behaviour and `GETTEXT_SYSTEM` still use static and
    dynamic linking respectively.

- `NUM_JOBS`: sets the number of parallel build jobs.

- `TMPDIR` (on Unix), `TMP`, `TEMP`, `USERPROFILE` (on Windows): set the
    parent directory for the temporary build directory.

    GNU gettext uses autotools, which [don't allow some characters][chars] in
    paths, notably a space character. To get around that, this crate performs
    the build in a temporary directory which usually resides somewhere under
    _/tmp_ or _C:\\Temp_. The aforementioned env vars allow you to move the
    build directory elsewhere.

    [chars]: https://www.gnu.org/software/autoconf/manual/autoconf-2.60/autoconf.html#Special-Chars-in-Variables

For target-specific configuration, each of these environment variables can be
prefixed by an upper-cased target, for example,
`X86_64_UNKNOWN_LINUX_GNU_GETTEXT_DIR`. This can be useful in cross compilation
contexts.

This doesn't work on AppVeyor ATM. Use `SET GETTEXT_SYSTEM=true` instead.
