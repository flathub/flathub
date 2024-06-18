# PDFStitcher Flatpak
This is the Flatpak repository for PDFStitcher. To see the source code, visit the [PDFStitcher repository](https://github.com/cfcurtis/pdfstitcher).

## Flatpak Permissions
By default, this Flatpak requests permission to access files in the user's home directory, as well as `/mnt`, `/media`, and `/run/media`. This allows PDFStitcher to access files on the user's computer, as well as files on external drives and network shares mounted in common locations.

If you would like to access files in different locations (e.g. for a custom network share mount point), try installing [Flatseal](https://flathub.org/apps/details/com.github.tchx84.Flatseal) and modifying the permissions for PDFStitcher, or use `flatpak override` to grant additional permissions.

## Oddities
While wxPython works with Wayland + GTK, a [hard-coded check](https://github.com/wxWidgets/Phoenix/issues/2557) prevents it from working with Flatpak. As a band-aid solution, this check is bypassed in the Flatpak build by overriding the `wx.PyApp.IsDisplayAvailable` method to always return `True`. This allows PDFStitcher to run on Wayland, but will also likely cause issues when running on a headless system (so, don't do that).

If you want a command-line only version of PDFStitcher, install with `pip3 install pdfstitcher` instead.