# Rimsort Flatpak

## To install
flatpak run org.flatpak.Builder --sandbox --user --force-clean --install --install-deps-from=flathub  --repo=repo builddir io.github.rimsort.yaml

## To run
flatpak run io.github.rimsort

## To debug

flatpak --command=bash run io.github.rimsort

flatpak run --command=sh --devel io.github.rimsort


## To make a bundle

flatpak build-bundle repo hello.flatpak io.github.rimsort --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo

flatpak build-bundle repo/ RimSortflatpak io.github.rimsort


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
