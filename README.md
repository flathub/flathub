# Flatpak qutebrowser

Here you will find the JSON manifest file to build qutebrowser in a flatpak.

There are two outstanding issues (that I know of):
- You neeed an internet connection during the build (which is not recommended) because not all dependencies are packaged and need to be installed through pip from PyPI.
- The settings are not persisent because they are saved only inside the flatpak container
