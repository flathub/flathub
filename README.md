# Dictionary App Builder Flatpak

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

Building the app from a clone of this repository. There's a `devbuild` file if you want to always uninstall the previous flatpak and save the output of the build process to a log file:
```bash
# Usage: ./devbuild [LOG_DIR] [SOURCE_DIR] [MANIFEST(w/out the .yml/.yaml/.json extension)]
./devbuild sab-build-logs ./ org.sil.dictionary-app-builder
```
Otherwise you can just run the build command on its own. This will also remove the previous flatpak by attempting to overwrite builds that have the exact same name and branch:
```bash
flatpak-builder --user --install --keep-build-dirs --force-clean build org.sil.dictionary-app-builder.yml
```
- The `--user` vs `--system` option installs to the machine's flatpak repo in either `~/.local/share/flatpak` for user or `/var/lib/flatpak` for system
- `--force-clean` empties the target directory
- `build` is the target directory

Each build will produce a cached copy of certain files in the manifest's local directory in `./.flatpak-builder/build/dictionary-app-builder`. Empty out `build/*` with `rm -r` before running another install if it's getting crowded

## Testing

Standard run commands from flatpak repo will launch dictionary-app-builder:
```bash
flatpak run org.sil.dictionary-app-builder
flatpak run org.sil.dictionary-app-builder [-COMMANDS]
```

- `-COMMANDS` implies that any build commands (pass the option '-?' for details) that can be specified for each app builder are automatically passed from flatpak to the app's run command (a bash script in this case). The run script in turn passes the args to the app's .jar file

Run terminal* instead of the app:
```bash
flatpak run --devel --command=bash org.sil.dictionary-app-builder
```

A useful app for testing / overwriting permissions after a build:
```bash
flatpak --user install flathub com.github.tchx84.Flatseal
```

## Exporting to other machines

### .flatpak Bundles

Exporting the app as a bundled .flatpak file:
```bash
# System location
# REPO_DIR="/var/lib/flatpak/repo"
# User Location
REPO_DIR="/home/user/.local/share/flatpak/repo/"
OUT_DIR="/media/user/Extended/artifacts"
base_package_name="dictionary-app-builder-${VERSION-$(date +"%F-%H%M%S")}"
mkdir -vp "${OUT_DIR}"

flatpak build-bundle "${REPO_DIR}" \
"${OUT_DIR}/${base_package_name}.flatpak" org.sil.dictionary-app-builder \
--runtime-repo=https://flathub.org/repo/flathub.flatpakrepo
```

Importing the bundle:**
```bash
# System location
# REPO_DIR="/var/lib/flatpak/app"
# User Location
REPO_DIR="/home/user/.local/share/flatpak/repo"
IN_DIR="/media/user/Extended/artifacts"

base_package_name="dictionary-app-builder-2024-07-18-210707"
flatpak build-import-bundle ${REPO_DIR} ${IN_DIR}/${base_package_name}.flatpak
```

### "usb" Repositories

Exporting apps and their runtime dependencies as a repo that can be loaded from a usb-drive. This is recommended by the documentation over bundles:**
- https://docs.flatpak.org/en/latest/usb-drives.html
- https://blogs.gnome.org/mclasen/2018/08/26/about-flatpak-installations/
```bash
# This must be run once to add flathub's collection ID to its repository
# config file. This is also needed for each repo that holds an app or
# accompanying runtime if you export multiple at once
flatpak remote-modify --collection-id=org.flathub.Stable flathub
flatpak update
```
```bash
flatpak create-usb --verbose --user --app /media/user/Extended org.sil.dictionary-app-builder
```

Importing from the "usb drive" repository:
```bash
#Target machine must have the same remote repository
flatpak install --sideload-repo=/media/user/Extended/.ostree/repo flathub org.sil.dictionary-app-builder
```

## Validation

Validation tools are specified by flatpak's linter which is included in flatpak-builder and can currently be used to validate manifests, metainfo, flatpak builds, and ostree repositories for builds:
- https://github.com/flathub-infra/flatpak-builder-lint#flatpak

Flatpak validate for manifest:
```bash
flatpak install flathub -y org.flatpak.Builder
#A blank terminal return is a good sign for this command
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest org.sil.dictionary-app-builder.yml
```
Flathub specific modification of `appstreamcli validate` for metainfo. This is recommended as it's more verbose regarding errors and warnings specific to flathub publishing:
- https://docs.flathub.org/docs/for-app-authors/metainfo-guidelines/validation
```bash
#You should see "Validation was successful"
flatpak run --command=flatpak-builder-lint org.flatpak.Builder appstream org.sil.dictionary-app-builder.metainfo.xml
```

A data checker to scan for new sources. This will perform something similar to what flathub runs automatically on published apps:
- https://github.com/flathub-infra/flatpak-external-data-checker
```bash
flatpak install --from https://dl.flathub.org/repo/appstream/org.flathub.flatpak-external-data-checker.flatpakref
```
```bash
#This assumes the manifest is somewhere under your /home directory.
#Add ' --filesystem=MANIFEST_DIR ' if this is not the case
flatpak run org.flathub.flatpak-external-data-checker MANIFEST_DIR/MANIFEST_FILE
```

## Clean up

You can safely delete these directories and still run the
flatpak from command line since `flatpak-builder` exported
the build to either a user or system flatpak repo in `~/.local/share/flatpak/app/org.sil.dictionary-app-builder` or `/var/lib/flatpak/app/org.sil.dictionary-app-builder` respectively.

However, the previous build's local target directory and its corresponding builder cache will be removed so a new build will take longer:

```
.flatpak-builder
build
dab-build-logs
```

To uninstall from the exported flatpak repo. You can remove the flatpak user data by adding the `--delete-data` option:
```bash
flatpak uninstall org.sil.dictionary-app-builder
```

To remove the runtimes the app relied on (and any other unused runtimes). If you run this when the app is still installed, the needed runtimes will be unaffected:
```bash
flatpak uninstall --unused
```

To remove user data of ALL uninstalled apps:
```bash
flatpak uninstall --delete-data
```

**(helpful for checking what variables and directories are available to the app. Look up flatpak-spawn if your only interested in just running a few commands in a flatpak's isolated environment)*

** *Had trouble getting these bundles to import successfully. Couldn't get local build repos with `create-usb` even after setting their collection ids but did have success if the app was already published under flathub's repo*