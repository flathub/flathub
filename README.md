# R2Modman Flathub submission

WIP Flathub submission for R2Modman.

Currently uses https://github.com/TB516/flathub-prep as the source repo until the changes are merged into the main repo.

## Building locally

For building and installing locally, Flatpak and Flatpak Builder are needed. Once those are installed, then you can run the following command in this directory to do a local build, and install it as a user level Flatpak: `flatpak-builder --user --install --force-clean dist io.github.ebkr.r2modman.yaml`

Once that is done, you should be able to launch R2Modman from your GUI, or run with `flatpak run io.github.ebkr.r2modman --enable-logging --trace-warnings --trace-uncaught` to get all logs in the console.
