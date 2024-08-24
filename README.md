# Maths Hurdler Activity Flatpak

A game designed to help teach children math in accordance with the Common Core standard for fourth grade students.

To know more refer https://github.com/sugarlabs/math-hurdler

## How To Build

```
git clone https://github.com/flathub/org.sugarlabs.MathHurdler.git
cd org.sugarlabs.MathHurdler
flatpak -y --user install flathub org.gnome.{Platform,Sdk}//46
flatpak -y --user install org.sugarlabs.BaseApp//24.04
flatpak-builder --user --force-clean --install build org.sugarlabs.MathHurdler.json
```

## Check For Updates

Install the flatpak external data checker
```
flatpak --user install org.flathub.flatpak-external-data-checker
```

Now to update every single module to the latest stable version use
```
cd org.sugarlabs.MathHurdler
flatpak run --filesystem=$PWD org.flathub.flatpak-external-data-checker org.sugarlabs.MathHurdler.json
```
