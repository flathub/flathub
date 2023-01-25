# RARS for Flatpak

[RARS (RISC-V Assembler and Runtime Simulator)](https://github.com/TheThirdOne/rars) created by [Benjamin Landers](https://github.com/TheThirdOne) packaged for [Flatpak](https://flatpak.org)

## License

* The RARS logo is under the [MIT license](https://github.com/TheThirdOne/rars/blob/master/License.txt).

## Documentation

### High DPI Screens

Set the `RARS_UISCALE` environment variable to a floating-point number at least `1.0` (eg: `2.0`,
`2.5`, `3.0`). This will set the `sun.java2d.uiScale` property (See documentation [here](https://news.kynosarges.org/2019/03/24/swing-high-dpi-properties/))

### Development
1. Install Flatpak (See [this](https://flatpak.org/setup/) guide)
2. Install Flatpak builder (`flatpak-builder`; see [this](https://docs.flatpak.org/en/latest/first-build.html) guide)
3. Install dependencies:
```shell
flatpak install org.freedesktop.Sdk.Extension.openjdk17 org.freedesktop.appstream-glib 
```
4. Build:
```shell
flatpak-builder build com.github.TheThirdOne.rars.yml --force-clean
```
5. Install:
```shell
flatpak-builder --user --install --force-clean build com.github.TheThirdOne.rars.yml
```
6. Run:
```shell
flatpak run com.github.TheThirdOne.rars
RARS_UISCALE=3.0 flatpak run com.github.TheThirdOne.rars
flatpak run com.github.TheThirdOne.rars h
```
7. Validate:
```shell
flatpak run org.freedesktop.appstream-glib validate com.github.TheThirdOne.rars.metainfo.xml
desktop-file-validate com.github.TheThirdOne.rars.desktop
```