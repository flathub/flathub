# Make Them Fall Activity Flatpak

Make Them Fall, is a challenging distance game that tests your multitasking skills. Control up to 6 falling stickmen and avoid spikes and obstacles. Choose from six difficulty levels and see how long you can last. Play alone or with friends for added fun!

To know more refer https://github.com/sugarlabs/make-them-fall-activity

## How To Build

```
git clone https://github.com/flathub/org.sugarlabs.MakeThemFall.git
cd org.sugarlabs.MakeThemFall
flatpak -y --user install flathub org.gnome.{Platform,Sdk}//46
flatpak -y --user install org.sugarlabs.BaseApp//24.04
flatpak-builder --user --force-clean --install build org.sugarlabs.MakeThemFall.json
```

## Check For Updates

Install the flatpak external data checker
```
flatpak --user install org.flathub.flatpak-external-data-checker
```

Now to update every single module to the latest stable version use
```
cd org.sugarlabs.MakeThemFall
flatpak run --filesystem=$PWD org.flathub.flatpak-external-data-checker org.sugarlabs.MakeThemFall.json
```
