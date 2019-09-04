# Unofficial flatpak for [Claws-Mail](https://claws-mail.org)

The flathub recipe for building Claws-mail as a flatpak distributable package.

Local build: `flatpak-builder --user --install --force-clean build org.claws_mail.Claws-Mail.json`

## TODO

Reminders for later consideration.

- FIXME: Investigate warning `** (claws-mail:2): WARNING **: 01:06:28.356: failed to open directory: /app/etc/skel/.claws-mail`
- FIXME: Figure out if we can be selective with persisting if `Mailbox` directories can be created at arbitrary locations. Otherwise transition to exposing full home directory.
- FIXME: Recommendations for `error: 'org.claws-mail.Claws-Mail' is not a valid application name: Only last name segment can contain -`
- FIXME: Align exact versions of dependencies with packaging instructions for debian/ubuntu by Claws-Mail developers.
- FIXME: create .desktop file.
- TODO: Check if we can integrate with NetworkManager. This feature is now disabled.
- TODO: Investigate if building for i386 is possible.
