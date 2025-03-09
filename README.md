# DSDA-Doom Flatpak port

This is a mostly complete configuration for DSDA-Doom for Flatpak. Before
submitting to Flathub:

- There's a change I made to i_system.c to move the data directory if running as
  Flatpak. Publishing of this should wait until this change is in a release.
- The YAML manifest currently points to my fork containing said change. This is
  so that the application works in its current state. This should be changed
  before submitting.
- The release date of the next release should be updated in the appdata.xml
  file before submitting.
- You might want to change the URLs of the screenshots in appdata.xml to point
  to your fork of this repository.

[More info about submitting here.](https://docs.flathub.org/docs/category/for-app-authors)

I haven't personally submitted anything to Flathub yet, so apologies if I missed
something!
