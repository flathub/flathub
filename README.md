# Betterbird (Flathub)

Betterbird is a fine-tuned version of [Mozilla Thunderbird](https://www.thunderbird.net/), Thunderbird on steroids, if you will.

[Betterbird](https://betterbird.eu/) for [Flatpak](https://flatpak.org/) installation instructions are available by [clicking here to visit the Betterbird app page on Flathub](https://flathub.org/apps/details/eu.betterbird.Betterbird).

## Known issues:
Non flatpak-packaging issues should be reported upstream at<br>
https://bugzilla.mozilla.org/describecomponents.cgi?product=Thunderbird

#### Migration from pre-exisiting non-flatpak installations
In order to migrate from pre-exisiting non-flatpak installation and preserve all settings please copy or move entire<br>
`~/.thunderbird`<br>
folder into<br>
`~/.var/app/org.mozilla.Thunderbird/.thunderbird`

In case Thunderbird opens a new profile instead of the existing one, run:<br>
`flatpak run org.mozilla.Thunderbird -P`<br>
then select the right profile and tick "*Use the selected profile without asking on startup*" box.

#### Language support
([#3](https://github.com/flathub/org.mozilla.Thunderbird/issues/3)) All supported locales are available in `org.mozilla.Thunderbird.Locale` extension. One locale that matches host OS locale will be installed and selected by default. For instructions about how to enable more locales in flatpak take a look at https://flatpak.readthedocs.io/en/latest/flatpak-command-reference.html#flatpak-config

([#90](https://github.com/flathub/org.mozilla.Thunderbird/issues/90)) Dictionaries availability is similar as for locales. They also could be downloaded manually from:<br>
https://addons.thunderbird.net/language-tools/<br>
and installed through:<br>
[Menu Bar](https://support.mozilla.org/kb/display-thunderbird-menus-and-toolbar) > `Tools` > `Add-ons` > `Extensions` > `Install Add-on From File`<br>
You may need to restart app in order to make changes effective.

#### New mail notifications
([#11](https://github.com/flathub/org.mozilla.Thunderbird/issues/11#issuecomment-531987872)) To enable new mail notifications:<br>
1. [Menu Bar](https://support.mozilla.org/kb/display-thunderbird-menus-and-toolbar) > `Edit` > `Preferences` > `Advanced` > `General` > `Config Editor…`, set `mail.biff.use_system_alert` to `true` (default)<br>
1. [Menu Bar](https://support.mozilla.org/kb/display-thunderbird-menus-and-toolbar) > `Edit` > `Preferences` > `General` > Select `Customize…` for "Show an alert" and set "Show New Mail alert for:"

([#79](https://github.com/flathub/org.mozilla.Thunderbird/issues/79#issuecomment-534298255)) Alternatively you may set `mail.biff.use_system_alert` to `false` which will make notifications non-native but clicking on them will open mail in Thunderbird.

#### Wayland
([#75](https://github.com/flathub/org.mozilla.Thunderbird/issues/75)) To enable the experimental [Wayland](https://wayland.freedesktop.org/) backend (assuming the desktop session runs under a Wayland) set:<br>
`flatpak override --user --env=MOZ_ENABLE_WAYLAND=1 org.mozilla.Thunderbird`

#### Smartcard
([#51](https://github.com/flathub/org.mozilla.Thunderbird/issues/51)) For Smartcard support you need at least Flatpak 1.3.2.

#### Other flatpak issues unresolved yet by upstream
([#123](https://github.com/flathub/org.mozilla.Thunderbird/issues/123)) Opening Profile Directory doesn't work: https://bugzilla.mozilla.org/show_bug.cgi?id=1625111
