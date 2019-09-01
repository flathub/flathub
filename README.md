# Unofficial flatpak for [Claws-Mail](https://claws-mail.org)

The flathub recipe for building Claws-mail as a flatpak distributable package.

Local build: `flatpak-builder --user --install --force-clean build org.clawsmail.Claws-Mail.json`

## TODO

Reminders for later consideration.

- TODO: Check if we can integrate with NetworkManager. This feature is now disabled.
- TODO: Figure out if we can be selective with persisting if `Mailbox` directories can be created at arbitrary locations. Otherwise transition to exposing full home directory.
- TODO: Recommendations for `error: 'org.claws-mail.Claws-Mail' is not a valid application name: Only last name segment can contain -`
- TODO? Extract bogofilter as shared-module? (shared with Evolution)
- TODO? Extract gdata as shared-module? (shared with Evolution)
- TODO? Extract libical as shared-module? (shared with Evolution)
- TODO: create .desktop file.
- TODO: Align exact versions of dependencies with packaging instructions for debian/ubuntu by Claws-Mail developers.
- TODO: Investigate if building for i386 is possible.
