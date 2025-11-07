## WPS 365 flatpak

[English](README.md)|[中文](README_CN.md)

### Important
- If the ui doesn't display in your language, edit or create `~/.var/app/cn.wps.wps_365/config/Kingsoft/Office.conf` and add the following lines at the beginning:
    ```
    [General]
    languages=LANGUAGE
    ```
    supported value of LANGUAGE includes `en_US`, `mn_CN`, `ru_RU`, `ug_CN` and `zh_CN`.
- If only the "Writer" icon can launch the software while "Spreadsheets" and "Presentation" do not respond, edit or create `~/.var/app/cn.wps.wps_365/config/Kingsoft/Office.conf` with
    ```
    [6.0]
    wpsoffice\Application%20Settings\AppComponentMode=prome_independ
    ```

### Tips
- If you’re concerned about potential privacy issues, you can disable network access by running: `flatpak --user override cn.wps.wps_365 --unshare=network`.
- The application will launch "wps cloud storage", and create an icon on system tray. If you uncheck "start with wps" in the settings of the tool, it will still be launched without the tray icon, making it impossible to gracefully exit the tool. In that case you will need `flatpak kill cn.wps.wps_365` to completely close the application.