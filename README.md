Flatpak Manifest for AppStream Generator
========================================

This repository contains the Flatpak manifest for the `appstream-generator` utility for
deployment via Flathub.
AppStream is a specification and set of tools to make machine-readable software metadata easily available
to programs that need it (e.g. software centers, firmware/driver updaters, font installers, etc.).
It also allows software authors to provide relevant user-centric information (e.g. a translated description,
screenshots, etc.) about their software in advance, so potential users can make an informed decision on
whether they want to install it.

This Flatpak bundle provides the AppStream Generator utility, which is capable of creating AppStream catalog
metadata from Linux distribution repositories (Debian, Ubuntu, Arch Linux, Fedora, ...) for deployment by those
distributions.
The tool will analyze metadata, render icons, cache screenshots, create HTML reports and much more. If you do
not need the advanced functionality of `appstream-generator` and do not have a distribution repository to generate
data for, using `appstreamcli compose` may be a lighter tool to use for a similar purpose.

You can run `appstream-generator` as Flatpak via:
`flatpak run org.freedesktop.appstream.generator`

To learn more about AppStream Generator, please visit the project's website at
[github.com/ximion/appstream-generator](https://github.com/ximion/appstream-generator).
