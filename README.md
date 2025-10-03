## About

This repository hosts files required for building a [Flatpak package](https://flatpak.org/) of [LRCGET](https://github.com/tranxuanthang/lrcget).

## For maintainers

### Regenerating source files

In this repository, there are three important files for building the app:

- `cargo-sources.json`
- `node-sources.json`
- `yarn.lock`

These files are required for building the app without network access at build time, because [Flathub does not allow it](https://docs.flathub.org/docs/for-app-authors/requirements#no-network-access-during-build).
This means that we must manually regenerate them whenever project dependencies change, otherwise the app stops building.

To help automate this task, the [`justfile`](https://just.systems/man/en/) in this repository can be used. But first, you need these tools installed:

- [just](https://github.com/casey/just) (To run the `justfile`)
- [git](https://git-scm.com/) (To clone [`flatpak-builder-tools`](https://github.com/flatpak/flatpak-builder-tools))
- [curl](https://curl.se/) (To download LRCGET's `package.json` and `Cargo.lock`)
- [Poetry](https://python-poetry.org/) (To run the [Cargo](https://github.com/flatpak/flatpak-builder-tools/tree/master/cargo) and [Node](https://github.com/flatpak/flatpak-builder-tools/tree/master/node) helpers from `flatpak-builder-tools`)
- [mikefarah's yq](https://github.com/mikefarah/yq) >= v4.x (To extract the `commit` property from the Flatpak manifest)
- [yarn](https://classic.yarnpkg.com/en/) v1.x (To generate a `yarn.lock`, which will be later used to generate `node-sources.json`)

Once those are installed, you can simply run:

```
just
```

And Bash scripts in the `justfile` will regenerate the required files.

To verify the app actually builds, you must build it locally without network access, using `flatpak-builder --sandbox`.
