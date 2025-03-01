# Rimsort Flatpak

## To install
flatpak run org.flatpak.Builder --sandbox --user --force-clean --install --install-deps-from=flathub  --repo=repo builddir io.github.rimsort.RimSort.yaml

## To run
flatpak run io.github.rimsort.RimSort

## To debug

flatpak --command=bash run io.github.rimsort.RimSort

flatpak run --command=sh --devel io.github.rimsort.RimSort


## To make a bundle

flatpak build-bundle repo hello.flatpak io.github.rimsort.RimSort --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo

flatpak build-bundle repo/ RimSortflatpak io.github.rimsort.RimSort

# to linter
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest io.github.rimsort.RimSort.yaml


# To fix
Autodetect folders

/home/yodatak/.local/share/Steam/steamapps/common/RimWorld

/home/yodatak/.config/unity3d/Ludeon Studios/RimWorld by Ludeon Studios/Config
/home/yodatak/.local/share/Steam/steamapps/workshop/content/294100
/home/yodatak/.local/share/Steam/steamapps/common/RimWorld/Mods


autodetect
https://github.com/RimSort/RimSort/blob/a3a5f451c02d8e726ed1643d0462eb33e3894537/app/controllers/settings_controller.py#L971

[WARNING][2025-02-22 01:01:40][2][MainThread][settings_controller][_on_locations_autodetect_button_clicked][981] : Auto-detected game folder path does not exist: /home/.../.steam/steam/steamapps/common/RimWorld/steamapps/common/RimWorld
[WARNING][2025-02-22 01:01:40][2][MainThread][settings_controller][_on_locations_autodetect_button_clicked][981] : Auto-detected config folder path does not exist: /home/.../.config/unity3d/Ludeon Studios/RimWorld by Ludeon Studios/Config
[WARNING][2025-02-22 01:01:40][2][MainThread][settings_controller][_on_locations_autodetect_button_clicked][981] : Auto-detected workshop mods folder path does not exist: /home/.../.steam/steam/steamapps/common/RimWorld/steamapps/workshop/content/294100
[WARNING][2025-02-22 01:01:40][2][MainThread][settings_controller][_on_locations_autodetect_button_clicked][981] : Auto-detected local mods folder path does not exist: /home/.../.steam/steam/steamapps/common/RimWorld/steamapps/common/RimWorld/Mods


ERROR: todds was not found. If you are running from source, please ensure you have followed the correct steps in the Development Guide:
https://github.com/RimSort/RimSort/wiki/Development-Guide

Please reach out to us for support at: https://github.com/oceancabbage/RimSort/issues


themes/default-icons/RimSort_Icon_64x64.svg

create a data folder and put desktop ?


### Locally testing the x-checker-data

Install flatpak-external-data-checker:

`flatpak install --from https://dl.flathub.org/repo/appstream/org.flathub.flatpak-external-data-checker.flatpakref`

Run flatpak-external-data-checker:

`flatpak run org.flathub.flatpak-external-data-checker io.github.rimsort.RimSort.yaml --update`
