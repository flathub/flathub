# Betterbird (Flathub)

Betterbird is a fine-tuned version of [Mozilla Thunderbird](https://www.thunderbird.net/), Thunderbird on steroids, if you will.

[Betterbird](https://betterbird.eu/) for [Flatpak](https://flatpak.org/) installation instructions are available by [clicking here to visit the Betterbird app page on Flathub](https://flathub.org/apps/details/eu.betterbird.Betterbird).


## Migration from pre-exisiting installations

#### Migration from pre-exisiting Thunderbird flatpak installations
In order to migrate from pre-exisiting Thunderbird flatpak installation and preserve all settings please copy or move entire<br>
`~/.var/app/org.mozilla.Thunderbird/.thunderbird`<br>
folder into<br>
`~/.var/app/eu.betterbird.Betterbird/.thunderbird`

#### Migration from pre-exisiting Thunderbird non-flatpak installations
In order to migrate from pre-exisiting non-flatpak Thunderbird installation and preserve all settings please copy or move entire<br>
`~/.thunderbird`<br>
folder into<br>
`~/.var/app/eu.betterbird.Betterbird/.thunderbird`

#### Migration from pre-exisiting Betterbird non-flatpak installations
In order to migrate from pre-exisiting non-flatpak Betterbird installation and preserve all settings please copy or move entire<br>
`~/.thunderbird`<br>
folder into<br>
`~/.var/app/eu.betterbird.Betterbird/.thunderbird`

In case Betterbird opens a new profile instead of the existing one, run:<br>
`flatpak run eu.betterbird.Betterbird -P`<br>
then select the right profile and tick "*Use the selected profile without asking on startup*" box.

## Known issues
#### Language support
The Betterbird flatpak currently only contains the English version of the software.

#### New mail notifications
([#11](https://github.com/flathub/org.mozilla.Thunderbird/issues/11#issuecomment-531987872)) To enable new mail notifications:<br>
1. [Menu Bar](https://support.mozilla.org/kb/display-thunderbird-menus-and-toolbar) > `Edit` > `Preferences` > `Advanced` > `General` > `Config Editor…`, set `mail.biff.use_system_alert` to `true` (default)<br>
1. [Menu Bar](https://support.mozilla.org/kb/display-thunderbird-menus-and-toolbar) > `Edit` > `Preferences` > `General` > Select `Customize…` for "Show an alert" and set "Show New Mail alert for:"

([#79](https://github.com/flathub/org.mozilla.Thunderbird/issues/79#issuecomment-534298255)) Alternatively you may set `mail.biff.use_system_alert` to `false` which will make notifications non-native but clicking on them will open mail in Thunderbird.

#### Wayland
([#75](https://github.com/flathub/org.mozilla.Thunderbird/issues/75)) To enable the experimental [Wayland](https://wayland.freedesktop.org/) backend (assuming the desktop session runs under a Wayland):<br>
1. Give the `eu.betterbird.Betterbird` flatpak the `--socket=wayland` permission, e.g. by using [Flatseal](https://flathub.org/apps/details/com.github.tchx84.Flatseal).
2. Run `flatpak override --user --env=MOZ_ENABLE_WAYLAND=1 org.mozilla.Thunderbird` to enable the Wayland backend.

#### Smartcard
([#51](https://github.com/flathub/org.mozilla.Thunderbird/issues/51)) For Smartcard support you need at least Flatpak 1.3.2.

#### Other flatpak issues unresolved yet by upstream
([#123](https://github.com/flathub/org.mozilla.Thunderbird/issues/123)) Opening Profile Directory doesn't work: https://bugzilla.mozilla.org/show_bug.cgi?id=1625111

## Bug Reporting / Support

Read [www.betterbird.eu/support/](https://www.betterbird.eu//support/). Here is an abridged version:

1. Thunderbird has 14.000+ bugs which won't be fixed as part of Betterbird.
1. First step: Check whether the bug exists in Thunderbird. If so, check whether it has been reported at [Bugzilla](https://bugzilla.mozilla.org/). If reported, please let us know the bug number. If not reported, either you or our project will need to report it (see following item).
1. If the bug is also in Thunderbird, let us know that it's important to you, please provide reasons why Betterbird should fix it and not upstream Thunderbird. We'll check whether we deem it "must have" enough to fix it considering the necessary effort.
1. If the bug is only in Betterbird, let us know, we'll endeavour to fix it asap, usually within days.
1. Common sense bug reporting rules apply: Bug needs to be reproducible, user needs to cooperate in debugging.
