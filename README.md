## Blink
This is the Qt version of Blink, a fully featured, easy to use SIP client
for Linux and Microsoft Windows.

Homepage: http://icanblink.com

## Features

The complete list of features and implemented standards are available at:

http://icanblink.com/features/

## Installation

Installation instructions can be found at:

http://icanblink.com/download/

## Support

For help on using Blink Qt go to http://icanblink.com/help/

## Changelog

The changelog is available at http://icanblink.com/changelog/

## Credits

 * AG Projects: http://ag-projects.com
 * NLnet foundation: http://nlnet.nl
 * IETF Community: http://www.ietf.org
 * SIP SIMPLE client SDK: http://sipsimpleclient.org

------

### Here follows some commands I use to test and prefer to keep together with the repo for quick copy and paste:
```bash
sudo apt-get -y install flatpak flatpak-builder
flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak -y install flathub org.kde.Platform//5.11
flatpak -y install flathub org.kde.Sdk//5.11
flatpak -y install flathub io.qt.qtwebkit.BaseApp//5.11

# BUILD
flatpak-builder --force-clean --repo=test-repo build-dir io.github.syco.blink.json

# TEST
flatpak -y remote-add --no-gpg-verify test-repo test-repo
flatpak -y --system install test-repo io.github.syco.blink
flatpak run io.github.syco.blink
flatpak -y remove io.github.syco.blink

# EXPORT
flatpak build-bundle test-repo Blink-3.2.1.flatpak io.github.syco.blink

# IMPORT
sudo flatpak -y install Blink-3.2.1.flatpak
sudo flatpak -y remove io.github.syco.blink

```

