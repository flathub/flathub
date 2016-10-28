This repo contains a working version of the slack x86-64 app packaged as a flatpak.

The app builds on uses the freedesktop.org runtime and the
[electron-flatpak-base-app](https://github.com/endlessm/electron-flatpak-base-app).
You will need both to build the app.
```
flatpak --user remote-add --from gnome https://sdk.gnome.org/gnome.flatpakrepo
flatpak --user install gnome org.freedesktop.Platform/x86_64/1.4 org.freedesktop.Sdk/x86_64/1.4
flatpak --user remote-add endless-electron-apps --from https://s3-us-west-2.amazonaws.com/electron-flatpak.endlessm.com/endless-electron-apps.flatpakrepo
flatpak --user install endless-electron-apps io.atom.electron.BaseApp/x86_64/master
```

To test this, do:
```
make
flatpak --user remote-add --no-gpg-verify local-slack repo
flatpak --user install local-slack com.slack.Slack
```
