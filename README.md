Linux Audio base application
============================

This is a base application for Flatpak Linux Audio app. The intent is
to be the base for building Linux Audio plugins flatpaks.

It currently supports:

- LV2 as a `org.freedesktop.LinuxAudio.Lv2Plugin` extension.
- DSSI as a `org.freedesktop.LinuxAudio.DssiPlugin` extension.
- LADSPA as a `org.freedesktop.LinuxAudio.LadspaPlugin` extension.
- VST (Linux) as a `org.freedesktop.LinuxAudio.VstPlugin` extension.
- VST3 as a `org.freedesktop.LinuxAudio.Vst3Plugin` extension.

Content
-------

The base app only provide the following to build plugins:

- `lv2`
- `dssi`

LADSPA and VST don't need this, and some LV2 plugins don't need
either.

Application using audio plugins
-------------------------------

Your app support audio plugins?

Add the extension points for plugins. Below is an example for LV2 plugins:

```
"add-extensions": {
  "org.freedesktop.LinuxAudio.Lv2Plugin": {
    "directory": "extensions/Lv2Plugins",
    "add-ld-path": "lib",
    "merge-dirs": "lv2",
    "subdirectories": true,
    "no-autodownload": true,
    "autodelete": true
  }
}
```

Change the `directory` (the mount point) for each kind of plugins.

For DSSI and LADSPA change the key and the `merge-dirs` to respectively:

For DSSI: `org.freedesktop.LinuxAudio.DssiPlugin` and `dssi`

For LADSPA: `org.freedesktop.LinuxAudio.LadspaPlugin` and `ladspa`

For Linux VST (ie VST compiled for Linux, not those running with
Wine): `org.freedesktop.LinuxAudio.LadspaPlugin` and `lxvst`.

And make sure the application find the LV2 plugins by putting the
following finish argument:

```
"--env=LV2_PATH=/app/extensions/Lv2Plugins/lv2"
```

For DSSI, LADSPA and VST it is the same change as above. It's actually
recommended to add them all if you support LV2 as using a LV2 plugin
like Carla, you can use the others formats.

The table below list things. The mount point is a sub directory to
`/app/extensions`. The subdir is a subdirectory in the mount point
that will have all the plugins as needed by the application host.


| Format     | Ext point name | mount point | subdir | env          |
+------------+----------------+-------------+--------+--------------+
| LV2        | Lv2Plugin      | Lv2Plugins  | lv2    | `LV2_PATH`   |
| DSSI       | DssiPlugin     | DssiPlugins | dssi   | `DSSI_PATH`  |
| LADSPA     | LadspaPlugin   | LaspaPlugins| ladspa | `LADSPA_PATH`|
| VST (Linux)| VstPlugin      | VstPlugins  | lxvst  | `LXVST_PATH` and `VST_PATH` |
| VST3       | Vst3Plugin     | Vst3Plugins | vst3   | `VST3_PATH`  |


Plugins
-------

You want to provide a Plugin as a Flatpak package? Build a
an extension, using the base app.

LV2: `org.freedesktop.LinuxAudio.Lv2Plugin`
DSSI: `org.freedesktop.LinuxAudio.DssiPlugin`
LADSPA: `org.freedesktop.LinuxAudio.LadspaPlugin`
VST: `org.freedesktop.LinuxAudio.VstPlugin`
VST3: `org.freedesktop.LinuxAudio.Vst3Plugin`


Versions
--------

Versions have to match.

| BaseApp | Freedesktop SDK |
+---------+-----------------+
| 20.04   | 19.08           |
