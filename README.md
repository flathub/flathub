Flatpak manifest for GNOME Inform 7
===================================

This repo contains a Flatpak manifest for GNOME Inform 7, currently at version
6M62.

Techincal notes
---------------

 * Flatpak requires that the application icon and desktop file names begin with
   the full name of the application; here I have used a single line in the
   manifest to rename the desktop file, but for consistency, I have used a patch
   and a shell line to rename the application icon, so that it appears correctly
   both in the applications menu and within Inform 7 itself.
 * The WebKit1 module is copied from the official Flathub manifest for
   org.gimp.GIMP, along with a patch for the version of WebKit1 used. This takes
   a very long time to compile, and copying it from another successful Flatpak
   manifest seemed the best way to ensure success without painful trial and
   error.
 * So far I have elected to give the app access to only the "Inform"
   subdirectory of the user's home folder. This is following the norm for
   popular Flathub applications to restrict host filesystem access to one or two
   subdirectories of the home folder.
 * This manifest has not yet been tested on architectures other than x86_64.
