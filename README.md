# Flatpak for [Wheel Wizard](https://github.com/TeamWheelWizard/WheelWizard)

This is the Flatpak build repository for Wheel Wizard.
Wheel Wizard is a convenient Mario Kart Wii mod manager and launcher,
purpose-built for the Retro Rewind custom track distribution
with several online features.

## Updating the sources

In order to update the source files, you can use the
`update-sources.sh` script provided in this repository:

```bash
$ ./update-sources --dotnet <dotnet_version> --freedesktop <freedesktop_version> --commit <wheelwizard_commit>
```

The script will patch all .NET, Freedesktop versions along with the commit
of WheelWizard.

**Dependencies**: `coreutils`, `diffutils`, `flatpak`, `git`, `grep`, `patch`, `python3`, `sed`, `yq`.

To actually build and install the Flatpak locally using the local sources,
you can use the `build-and-install-local.sh` script:

```bash
$ ./build-and-install-local.sh
```
