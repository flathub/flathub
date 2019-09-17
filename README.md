# Unofficial flatpak for Claws-Mail

The flathub recipe for building [Claws-Mail](https://claws-mail.org) as a flatpak distributable package.

## Dependencies

The dependencies are as follows. In addition, the dependencies are in-order in the [Claws-Mail manifest](org.claws_mail.Claws-Mail.json).

Claws-Mail:
- libetpan

Plug-ins:
- TNEF
  - libytnef
- PDF-viewer
  - libpoppler
    - libopenjpeg
- VCalendar
  - libical
- Lite HTML-viewer
  - libgumbo
- GData
  - liboauth
  - libuhttpmock
- Bogofilter
  - bogofilter (cli filter application)

Disabled plug-ins:
- Dillo
- BSFilter
- Perl

## TODO

Reminders for later consideration.

- FIXME: add screenshots in prescribed resolution.
- FIXME: align exact versions of dependencies with packaging instructions for debian/ubuntu by Claws-Mail developers.
- FIXME: create .desktop file.
- FIXME: update URLs for screenshots to newly created `flathub/org.claws_mail.Claws-Mail` repository upon creation.
- TODO: Check if we can skip building documentation for _gdata_ dependency.
- TODO: Check if we can integrate with NetworkManager. This feature is now disabled.
- TODO: Investigate if building for i386 is possible.

<!-- NOTES

## References

- [Flatpak manifest permissions](http://docs.flatpak.org/en/latest/sandbox-permissions.html)
- [AppStream metadata (appdata.xml)](https://www.freedesktop.org/software/appstream/docs/sect-Metadata-Application.html)

-->
