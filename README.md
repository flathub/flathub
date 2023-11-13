Uhhyou plugins
==============

Build process.

gtkmm is install for build only. It seems to be required because
reason. It's not used by the plugins.

The build process follow upstream recommendations.

Cmake options:

- -DSMTG_RUN_VST_VALIDATOR=OFF so that the validator isn't run as it crashes.
- -DVSTGUI_STANDALONE=OFF
- -DVSTGUI_DISABLE_UNITTESTS=OFF
- -DVSTGUI_TOOLS=OFF neither 3 are needed.

VST patches: standard patches for -lfs or gcc 13 headers.
