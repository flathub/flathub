#!/bin/sh

cat << EOF
=== Begin org.videolan.VLC.Plugin.pause_click Usage Instructions ===
You have installed org.videolan.VLC.Plugin.pause_click.
This plugin must be enabled in order for it to work.

(Recommended) You can enable it by using the following steps after the VLC launches:
  1. Tick "Pause/Play video on mouse click" checkbox in Tools -> Preferences -> Show settings -> All -> Interface -> Control Interfaces. Screenshot: https://i.imgur.com/m9yF5Px.png
  2. Tick "Pause/Play video on mouse click" checkbox in Tools -> Preferences -> Show settings -> All -> Video -> Filters. Screenshot: https://i.imgur.com/OZLqmI6.png
  3. Restart VLC
Note that both checkboxes need to be ticked for the plugin to function!

(Alternative) You can pass:
  --control=pause_click --video-filter=pause_click
arguments when running VLC to enable the plugin.
This method enables the plugin only for this specific VLC run, i.e. it's not persistent.

Plugin settings are available under Tools -> Preferences -> Show settings -> All -> Video -> Filters -> Pause click. Screenshot: https://i.imgur.com/Kdrekks.png . The settings changes apply right away, there is no need to restart VLC.
=== End org.videolan.VLC.Plugin.pause_click Usage Instructions ===
EOF
