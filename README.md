# Calculate Activity Flatpak

This is the place to get the answer to a quick problem, but that is not the limit! You can also explore Algebra, Trigonometry, Boolean and more!

To know more refer https://github.com/sugarlabs/calculate-activity

## How To Build

```
git clone https://github.com/flathub/org.sugarlabs.Calculate.git
cd org.sugarlabs.Calculate
flatpak -y --user install flathub org.gnome.{Platform,Sdk}//46
flatpak -y --user install org.sugarlabs.BaseApp//24.04
flatpak-builder --user --force-clean --install build org.sugarlabs.Calculate.json
```

## Check For Updates

Install the flatpak external data checker
```
flatpak --user install org.flathub.flatpak-external-data-checker
```

Now to update every single module to the latest stable version use
```
cd org.sugarlabs.Calculate
flatpak run --filesystem=$PWD org.flathub.flatpak-external-data-checker org.sugarlabs.Calculate.json
```
