
# for build flatpak
flatpak-builder flatpakbuild net.damonlynch.rapid_photo_downloader.yaml  --force-clean
flatpak-builder --user --install --force-clean flatpakbuild net.damonlynch.rapid_photo_downloader.yaml
flatpak run net.damonlynch.rapid_photo_downloader
flatpak run net.damonlynch.rapid_photo_downloader --detailed-version



latpak run org.flathub.flatpak-external-data-checker net.damonlynch.rapid_photo_downloader.yaml
