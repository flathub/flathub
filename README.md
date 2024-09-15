# Flipsticks Activity Flatpak

Flipsticks is a keyframe animation activity that lets you pose and program a stick figure to walk, run, rotate, twist, tumble and dance. You can save your animations to the journal and will soon be able to share them via the mesh. Flipsticks can be used to explore concepts in geometry, computer programming and animation; it helps develop spatial and analytical thinking skills.

To know more refer https://github.com/sugarlabs/flipsticks.git

## How To Build

```
git clone https://github.com/flathub/org.sugarlabs.FlipSticks.git
cd org.sugarlabs.FlipSticks
flatpak -y --user install flathub org.gnome.{Platform,Sdk}//46
flatpak -y --user install org.sugarlabs.BaseApp//24.04
flatpak-builder --user --force-clean --install build org.sugarlabs.FlipSticks.json
```

## Check For Updates

Install the flatpak external data checker
```
flatpak --user install org.flathub.flatpak-external-data-checker
```

Now to update every single module to the latest stable version use
```
cd org.sugarlabs.FlipSticks
flatpak run --filesystem=$PWD org.flathub.flatpak-external-data-checker org.sugarlabs.FlipSticks.json
```
