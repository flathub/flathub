# Will not work. This is more of just a note for later.

flatpak-node-generator -o packages-panmirror.json yarn https://raw.githubusercontent.com/quarto-dev/quarto/refs/heads/release/rstudio-cherry-blossom/yarn.lock
flatpak-node-generator -o packages-rstudio.json npm https://github.com/rstudio/rstudio/blob/v2024.09.0%2B375/src/node/desktop/package-lock.json
