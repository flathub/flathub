# Postman with Git (Flatpak app)

Build and deploy locally:
1. Add `flathub` repo to the user scope:
   `flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo`
2. Build and install locally:
   `flatpak-builder --force-clean --user --repo=repo --install build io.github.yaal.PostmanGit.yaml`