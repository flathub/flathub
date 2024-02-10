<!--
  SPDX-FileCopyrightText: 2024 Junde Yhi <junde@yhi.moe>
  SPDX-License-Identifier: CC0-1.0
-->

# GNAT Ada Compiler as Freedesktop SDK Extension

This folder contains necessary files to repack an GNAT-based Ada development environment into an extention to the `org.freedesktop.Sdk` Flatpak container.

## Includes

- GNAT-FSF (Ada/GCC)
- GNATprove (SPARK)
- GPRbuild
- Alire (`alr`)

So far the manifest simply downloads pre-built binaries from [alire-project/GNAT-FSF-builds](https://github.com/alire-project/GNAT-FSF-builds/releases).

## License

The metadata files included in this folder are public domain work under the CC0 1.0 license. See [CC0-1.0.txt](./LICENSES/CC0-1.0.txt) for a copy of the license text.
