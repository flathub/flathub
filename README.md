# R2Modman Flathub submission

WIP Flathub submission for R2Modman.

Currently uses https://github.com/TB516/flathub-prep as the source repo until the changes are merged into the main repo.

## Building locally

For building and installing locally, Flatpak and Flatpak Builder are needed. Flatpak Builder (along with some extra tooling) can be gotten with the `org.flatpak.Builder` Flatpak.

### Building

To build and install the Flatpak, this command can be ran: `flatpak run --command=flatpak-builder org.flatpak.Builder --user --install --force-clean dist io.github.ebkr.r2modman.yaml`

### Linting

To lint the manifest, this command can be ran: `flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest io.github.ebkr.r2modman.yaml`

To lint the appstream metainfo, this command can be ran: `flatpak run --command=flatpak-builder-lint org.flatpak.Builder appstream io.github.ebkr.r2modman.metainfo.xml`
