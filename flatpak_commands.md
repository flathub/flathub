flatpak run --command=flatpak-builder-lint org.flatpak.Builder appstream io.github.ychsue.nonblockinglife.metainfo.xml

flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo

flatpak uninstall io.github.ychsue.nonblockinglife
rm -rf ~/.var/app/io.github.ychsue.nonblockinglife
rm -rf build-dir
rm -rf builddir
rm -rf repo
rm -rf .flatpak-builder

flatpak run org.flatpak.Builder --force-clean --repo=repo --compose-url-policy=full --mirror-screenshots-url=https://dl.flathub.org/media build-dir io.github.ychsue.nonblockinglife.yml


flatpak run --command=flathub-build org.flatpak.Builder --install io.github.ychsue.nonblockinglife.yml

flatpak run io.github.ychsue.nonblockinglife

flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest io.github.ychsue.nonblockinglife.yml

flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo

# 實驗

