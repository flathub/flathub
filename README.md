# Hedge Mod Manager for Flathub

Repository containing information and scripts to update and build [Hedge Mod Manager](https://github.com/hedge-dev/HedgeModManager). 

## For Collaborators
To update the software you must first on the upstream repo update the [metainfo file](https://github.com/hedge-dev/HedgeModManager/blob/flatpak/hedgemodmanager.metainfo.xml) with the new release information, then tag the release. After that is done, update the commit hash within this repo's manifest file with the commit hash matching the new release's tag.

If any NuGet packages were changed since the last update, you must download the `nuget-sources.json` file from the CI job for the new release and update it on this repo. You may also run `pull-latest-nuget-sources.sh` to pull the NuGet sources and `build-and-install.sh` to ensure the software builds and installs correctly.

You will need to commit all the changes to a new branch then create a PR against the main branch.
