# Falabracman Activity Flatpak

Falabracman-activity is a Sugar activity to learn some words, by collecting all the letters in the right order and by avoiding the lake otherwise you will fall into it.

To know more refer https://github.com/sugarlabs/falabracman-activity

## How To Build

```
git clone https://github.com/flathub/org.sugarlabs.Falabracman.git
cd org.sugarlabs.Falabracman
flatpak -y --user install flathub org.gnome.{Platform,Sdk}//46
flatpak -y --user install org.sugarlabs.BaseApp//24.04
flatpak-builder --user --force-clean --install build org.sugarlabs.Falabracman.json
```

## Check For Updates

Install the flatpak external data checker
```
flatpak --user install org.flathub.flatpak-external-data-checker
```

Now to update every single module to the latest stable version use
```
cd org.sugarlabs.Falabracman
flatpak run --filesystem=$PWD org.flathub.flatpak-external-data-checker org.sugarlabs.Falabracman.json
```
