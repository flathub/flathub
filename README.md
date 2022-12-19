# E:D Market Connector Flatpak

This is the Flatpak build of the [E:D Market Connector](https://github.com/EDCD/EDMarketConnector),
a helpful tool for the space flight simulation game [Elite Dangeous](https://www.elitedangerous.com/).

For more information on configuration and usage, see the [EDMC Wiki](https://github.com/EDCD/EDMarketConnector/wiki).

## Journal files location

On the first start you have to manually select the location of the game journal files. If you are using the default
install location, these are located in

```plain
~/.steam/steam/steamapps/compatdata/359320/pfx/drive_c/users/steamuser/Saved Games/Frontier Developments/Elite Dangerous
```

If you installed the game in a different directory, you first need to allow the E:D Market Connector Flatpak to access
that directory. To do that, install the [Flatseal Flatpak](https://flathub.org/apps/details/com.github.tchx84.Flatseal)
to modify the permissions and add the folder in the `Filesystem -> Other files` section.

## Flatpak specific paths

Flatpak apps store their own data in different locations than a normal app on your host. You can find them here:

- Config:  

  ```plain
  ~/.var/app/io.edcd.EDMarketConnector/config/EDMarketConnector/EDMarketConnector.ini
  ```

- Plugins:  

  ```plain
  ~/.var/app/io.edcd.EDMarketConnector/data/EDMarketConnector/plugins/
  ```
