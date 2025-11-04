## WPS 365 flatpak

[English](README.md)|[中文](README_CN.md)

### 提示
- 如果担心隐私问题，可以通过`flatpak --user override cn.wps.wps_365 --unshare=network`关闭网络权限.
- WPS 365 会在启动时启动 WPS 网盘并创建托盘图标。如果你在 WPS 网盘设置中关闭“WPS 启动时自动启动”，它依然会启动，只是不会创建托盘图标，导致无法用右键退出。在这种情况下你需要使用`flatpak kill cn.wps.wps_365`才能彻底关闭程序.
- 当“窗口管理模式”为“整合模式”时，组件的 desktop 文件无法启动应用。这是上游 BUG。如果你希望使用组件的 desktop 文件启动应用，你必须将“窗口管理模式”切换为“多组件模式”。如果你甚至无法进入图形界面，创建或编辑`~/.var/app/cn.wps.wps_365/config/Kingsoft/Office.conf`：
    ```
    [6.0]
    wpsoffice\Application%20Settings\AppComponentMode=prome_independ
    ```
