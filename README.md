## Steps to Build flatpak of the rolisteam client/server 

This requires that [flatpak-builder](https://github.com/flatpak/flatpak-builder) is installed. 

Copy the contents of the flatpak directory into a build area.

Run:

    flatpak-builder --force-clean org.rolisteam.rolisteam  rolisteam.json

This will build the sandbox.

To install into a local repositry you need first to create one.

    flatpak-builder --repo=.repo --ccache --force-clean .app-dir rolisteam.json

And then to add it

    flatpak --user remote-add --no-gpg-verify local-test-remote $PWD/.repo


then to install the package.

    flatpak --user install local-test-remote org.rolisteam.rolisteam

This will add it to your desktop search and so you should be able to search rolisteam and find it to run it. But to run it by hand use:

    flatpak run org.rolisteam.rolisteam


### Note. 

The appdata.xml file is used to populate the flathub page and so should be multilingual and have better screenshots


The desktop file too need to be multilingual. 

Good examples can be found in the Gnome's Soundjuicer's files  

`/usr/share/applications/org.gnome.SoundJuicer.desktop`

`/usr/share/appdata/org.gnome.SoundJuicer.appdata.xml`
