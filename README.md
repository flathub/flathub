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
flatpak -y install flathub org.kde.Platform//5.14
flatpak -y install flathub org.kde.Sdk//5.14
flatpak -y install flathub io.qt.qtwebkit.BaseApp//5.14

# BUILD
flatpak-builder --force-clean --repo=test-repo build-dir com.icanblink.blink.json

# TEST
flatpak -y remote-add --no-gpg-verify test-repo test-repo
flatpak -y --system install test-repo com.icanblink.blink
flatpak run com.icanblink.blink
flatpak -y remove com.icanblink.blink

# EXPORT
flatpak build-bundle test-repo Blink-3.2.1.flatpak com.icanblink.blink

# IMPORT
sudo flatpak -y install Blink-3.2.1.flatpak
sudo flatpak -y remove com.icanblink.blink

```

## flatpak-pip-generator
```bash
git clone https://github.com/flatpak/flatpak-builder-tools.git /root/flatpak-builder-tools

mkdir pip
/root/flatpak-builder-tools/pip/flatpak-pip-generator --python2 --no-build-isolation NO_BUILD_ISOLATION pip --output pip/pip
/root/flatpak-builder-tools/pip/flatpak-pip-generator --python2 --no-build-isolation NO_BUILD_ISOLATION setuptools --output pip/setuptools
/root/flatpak-builder-tools/pip/flatpak-pip-generator --python2 --no-build-isolation NO_BUILD_ISOLATION wheel --output pip/wheel
/root/flatpak-builder-tools/pip/flatpak-pip-generator --python2 --no-build-isolation NO_BUILD_ISOLATION Cython --output pip/Cython
/root/flatpak-builder-tools/pip/flatpak-pip-generator --python2 --no-build-isolation NO_BUILD_ISOLATION cffi --output pip/cffi
/root/flatpak-builder-tools/pip/flatpak-pip-generator --python2 --no-build-isolation NO_BUILD_ISOLATION dnspython --output pip/dnspython
/root/flatpak-builder-tools/pip/flatpak-pip-generator --python2 --no-build-isolation NO_BUILD_ISOLATION gmpy2 --output pip/gmpy2
/root/flatpak-builder-tools/pip/flatpak-pip-generator --python2 --no-build-isolation NO_BUILD_ISOLATION google-api-python-client --output pip/google-api-python-client
/root/flatpak-builder-tools/pip/flatpak-pip-generator --python2 --no-build-isolation NO_BUILD_ISOLATION greenlet --output pip/greenlet
/root/flatpak-builder-tools/pip/flatpak-pip-generator --python2 --no-build-isolation NO_BUILD_ISOLATION incremental --output pip/incremental
/root/flatpak-builder-tools/pip/flatpak-pip-generator --python2 --no-build-isolation NO_BUILD_ISOLATION lxml --output pip/lxml
/root/flatpak-builder-tools/pip/flatpak-pip-generator --python2 --no-build-isolation NO_BUILD_ISOLATION oauth2client --output pip/oauth2client
/root/flatpak-builder-tools/pip/flatpak-pip-generator --python2 --no-build-isolation NO_BUILD_ISOLATION python-application --output pip/python-application
/root/flatpak-builder-tools/pip/flatpak-pip-generator --python2 --no-build-isolation NO_BUILD_ISOLATION python-dateutil --output pip/python-dateutil
/root/flatpak-builder-tools/pip/flatpak-pip-generator --python2 --no-build-isolation NO_BUILD_ISOLATION python-gnutls --output pip/python-gnutls
/root/flatpak-builder-tools/pip/flatpak-pip-generator --python2 --no-build-isolation NO_BUILD_ISOLATION python-otr --output pip/python-otr
/root/flatpak-builder-tools/pip/flatpak-pip-generator --python2 --no-build-isolation NO_BUILD_ISOLATION Twisted --output pip/Twisted

```

