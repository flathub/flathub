Linux Audio base application
============================

This is a base application for Flatpak Linux Audio app. The intent is
to have a common base to provide Linux Audio plugins.

It currently supports:

- LV2 as a `org.freedesktop.LinuxAudio.Lv2Plugin` extension.
- DSSI as a `org.freedesktop.LinuxAudio.DssiPlugin` extension.
- LADSPA as a `org.freedesktop.LinuxAudio.LadspaPlugin` extension.

Content
-------

The base app is uses the KDE runtime to provide Qt5, as Gtk3 is
already part of freedesktop.org.

It builds `gtkmm2` and the corresponding `gtk2`.

For plugin support, it builds:

- `lv2`
- `liblo`
- `dssi`
- `ladspa`
- `lrdf`

Application using audio plugins
-------------------------------

Your app support audio plugins? Build it using this BaseApp.

The manifest must have the following:
```
"base": "org.freedesktop.LinuxAudio.BaseApp",
"base-version": "20.04",
"sdk": "org.kde.Sdk",
"runtime": "org.kde.Platform",
"runtime-version": "5.14",
```

And you must also add this extension point for LV2 plugins:

```
"add-extensions": {
  "org.freedesktop.LinuxAudio.Lv2Plugin": {
    "directory": "extensions",
    "add-ld-path": "lib",
    "merge-dirs": "lv2",
    "subdirectories": true,
    "no-autodownload": true,
    "autodelete": true
  }
}
```

For DSSI and LADSPA change the key and the `merge-dirs` to respectively:

For DSSI: `org.freedesktop.LinuxAudio.DssiPlugin` and `dssi`

For LADSPA: `org.freedesktop.LinuxAudio.LadspaPlugin` and `ladspa`

And make sure the application find the LV2 plugins by putting the
following finish argument:

```
"--env=LV2_PATH=/app/extensions/lv2"
```

For DSSI and LADSPA it is the same change as above.

If your application needs gtk2, remove it from the manifest. Remove
lv2, liblo, dssi, ladspa, lrdf as well.

Plugins
-------

You want to provide a Plugin as a Flatpak package? Build a
an extension.

LV2: `org.freedesktop.LinuxAudio.Lv2Plugin`
DSSI: `org.freedesktop.LinuxAudio.DssiPlugin`
LADSPA: `org.freedesktop.LinuxAudio.LadspaPlugin`
