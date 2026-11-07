## [2026-11-07] 改用 ManagedBass 代替 VLC與去除多餘的套件

## [2026-11-03] Try to fix the error happened on Flathub build
1. 由於 flathub 上面編譯出現 `Unable to find package Microsoft.NETCore.App.Host.linux-x64` 的錯誤，因此，在 nuget-sources.json 中加入缺少的套件。
2. 同時加入 `runtime.linux-x64.microsoft.dotnet.ilcompiler` 套件免得可能使用到AOT編輯器元件。
3. 重新砍掉本機上的 `.flatpak-builder`, `builddir` 與 `repo`，再在本機上跑
``` bash
flatpak run --command=flathub-build org.flatpak.Builder --install io.github.ychsue.nonblockinglife.yml

flatpak run io.github.ychsue.nonblockinglife

flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest io.github.ychsue.nonblockinglife.yml

flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo
```
依舊沒看到錯誤，不曉得送上去之後，flathub 會不會說有其他的問題。

## [2026-11-02] 今天拍影片好滿足 flathub 的要求

## [2026-11-01] 第一次 commit 到 flathub