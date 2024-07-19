## Keyboard App Builder Flatpak

## Building:

If running flatpak for the first time, install flatpak and default location for repo's:
```bash
#'sudo apt install flatpak' didn't work on my 20.04 build so use
#this older method if needed:
sudo add-apt-repository ppa:flatpak/stable
sudo apt update
sudo apt install flatpak
```
Install Flathub repo:
```bash
flatpak --user remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
```

An OS restart is apparently required after installing the repo in a first-time setup...

### Dependencies:

Install Flatpak-Builder:
```bash
sudo apt install flatpak-builder flatpak
```

Install freedesktop runtime, and openjdk:
```bash
flatpak --user install flathub org.freedesktop.Sdk//23.08
flatpak --user install flathub org.freedesktop.Platform//23.08
flatpak --user install flathub org.freedesktop.Sdk.Extension.openjdk17//23.08
flatpak update
```

### Additional Files:

A very useful script I used for generating the "requests" python module dependency. Its parent repository is full of tools for preparing build manifests for common software:

- https://github.com/flatpak/flatpak-builder-tools/tree/master/pip

Repo for the script that pulls from a docker registry without having docker installed:

- https://github.com/NotGlop/docker-drag 


### Build

Building the app from a clone of this repository. This will overwrite previous builds that have the exact same name and branch:
```bash
flatpak-builder --user --install --keep-build-dirs --force-clean build org.sil.keyboard-app-builder.yml
```
- The `--user` vs `--system` option installs to the machine's flatpak repo in either `~/.local/share/flatpak` for user or `/var/lib/flatpak` for system
- `--force-clean` empties the target directory
- `build` is the target directory

Each build will produce a cached copy of certain files in the manifest's local directory in `./.flatpak-builder/build/keyboard-app-builder`. Empty out `build/*` with `rm -r` before running another install if it's getting crowded

## Testing

Standard run commands from flatpak repo will launch keyboard-app-builder:
```bash
flatpak run org.sil.keyboard-app-builder
flatpak run org.sil.keyboard-app-builder [-COMMANDS]
```

- `-COMMANDS` implies that any build commands (pass the option '-?' for details) that can be specified for each app builder are automatically passed from flatpak to the app's run command (a bash script in this case). The run script in turn passes the args to the app's .jar file

Run terminal* instead of the app:
```bash
flatpak run --devel --command=bash org.sil.keyboard-app-builder
```


### Validation

Validation tools are specified by flatpak's linter which is included in flatpak-builder and can currently be used to validate manifests, metainfo, flatpak builds, and ostree repositories for builds:
- https://github.com/flathub-infra/flatpak-builder-lint#flatpak

Flatpak validate for manifest:
```bash
flatpak install flathub -y org.flatpak.Builder
#A blank terminal return is a good sign for this command
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest org.sil.keyboard-app-builder.yml
```
Flathub specific modification of `appstreamcli validate` for metainfo. This is recommended as it's more verbose regarding errors and warnings specific to flathub publishing:
- https://docs.flathub.org/docs/for-app-authors/metainfo-guidelines/validation
```bash
#You should see "Validation was successful"
flatpak run --command=flatpak-builder-lint org.flatpak.Builder appstream org.sil.keyboard-app-builder.metainfo.xml
```

### Clean up

You can safely delete these directories and still run the
flatpak from command line since `flatpak-builder` exported
the build to either a user or system flatpak repo in `~/.local/share/flatpak/app/org.sil.keyboard-app-builder` or `/var/lib/flatpak/app/org.sil.keyboard-app-builder` respectively.

However, the previous build's local target directory and its corresponding builder cache will be removed so a new build will take longer:

```
.flatpak-builder
build
```

To uninstall from the exported flatpak repo:
```bash
flatpak uninstall org.sil.keyboard-app-builder
```

To remove the runtimes the app relied on (and any other unused runtimes). If you run this when the app is still installed, the needed runtimes will be unaffected:
```bash
flatpak uninstall --unused
```

**(helpful for checking what variables and directories are available to the app. Look up flatpak-spawn if your only interested in just running a few commands in a flatpak's isolated environment)*
