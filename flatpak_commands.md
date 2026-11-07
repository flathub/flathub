flatpak run --command=flatpak-builder-lint org.flatpak.Builder appstream io.github.ychsue.nonblockinglife.metainfo.xml

flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo

flatpak uninstall io.github.ychsue.nonblockinglife
rm -rf ~/.var/app/io.github.ychsue.nonblockinglife
rm -rf build-dir
rm -rf builddir
rm -rf repo
rm -rf .flatpak-builder

flatpak-builder --force-clean build-dir io.github.ychsue.nonblockinglife.yml
flatpak run --command=flathub-build org.flatpak.Builder --install io.github.ychsue.nonblockinglife.yml

flatpak run io.github.ychsue.nonblockinglife

flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest io.github.ychsue.nonblockinglife.yml

flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo

# 實驗
flatpak-builder --force-clean --state-dir=flatpak_state build-dir io.github.ychsue.nonblockinglife.yml
## 清除 build-dir
rm -rf build-dir
## 如果想澈底清除模組編譯快取，但保留下載，可以只刪除 flatpak_state 裡的 build 快取
rm -rf flatpak_state/build flatpak_state/ccache
## 重新建置應用程式
flatpak-builder --force-clean --state-dir=flatpak_state build-dir io.github.ychsue.nonblockinglife.yml
flatpak run org.flatpak.Builder --force-clean --install --user --state-dir=flatpak_state build-dir io.github.ychsue.nonblockinglife.yml
