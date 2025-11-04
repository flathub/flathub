## WPS 365 flatpak

[English](README.md)|[中文](README_CN.md)

### Tips
- If you’re concerned about potential privacy issues, you can disable network access by running: `flatpak --user override cn.wps.wps_365 --unshare=network`.
- The application will launch "wps cloud storage", and create an icon on system tray. If you uncheck "start with wps" in the settings of the tool, it will still be launched without the tray icon, making it impossible to gracefully exit the tool. In that case you will need `flatpak kill cn.wps.wps_365` to completely close the application.
- When the "Window Management Mode" is set to "Integrated Mode", the component’s desktop files cannot be used to launch the application. This is an upstream bug. If you want to launch the application using a component’s desktop file, you must switch the "Window Management Mode" to "Multi-Component Mode" in settings. If you can't launch the gui, you can also edit or create `~/.var/app/cn.wps.wps_365/config/Kingsoft/Office.conf` with
    ```
    [6.0]
    wpsoffice\Application%20Settings\AppComponentMode=prome_independ
    ```