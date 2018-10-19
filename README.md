# Lutris Flatpak

An attempt at getting Lutris to run as a [Flatpak](https://flatpak.org/).

## Build

To compile Lutris as a Flatpak, you'll need both [Flatpak](https://flatpak.org/) and [Flatpak Builder](http://docs.flatpak.org/en/latest/flatpak-builder.html) installed. Once you manage that, do the following...

1. Add the platform dependencies...
  ```
  flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
  flatpak install --user flathub org.freedesktop.Sdk//18.08
  flatpak install --user flathub org.freedesktop.Platform//18.08
  ```

2. Compile and install the Flatpak...
  ```
  flatpak-builder --user --repo=lutris --force-clean build-dir org.lutris.Lutris.yaml
  flatpak remote-add --user lutris lutris --no-gpg-verify
  flatpak install --user lutris org.lutris.Lutris
  ```

3. Run it...
  ```
  flatpak run org.lutris.Lutris
  ```

## Clean up

```
flatpak uninstall --user org.lutris.Lutris
rm -rf ~/.var/app/org.lutris.Lutris .flatpak-builder
flatpak remote-delete lutris
```

## Development

The Python packages are built with https://github.com/flatpak/flatpak-builder-tools/tree/master/pip
