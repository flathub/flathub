# Typing Turtle Activity Flatpak

Need some help typing? Here you will learn the best way to hold your hands in order for you to become a faster typist! It gradually introduces the keys on the keyboard through a series of lessons, until the student has learned the entire keyboard.

To know more refer https://github.com/sugarlabs/typing-turtle-activity

## How To Build

```
git clone https://github.com/flathub/org.sugarlabs.TypingTurtle.git
cd org.sugarlabs.TypingTurtle
flatpak -y --user install flathub org.gnome.{Platform,Sdk}//46
flatpak -y --user install org.sugarlabs.BaseApp//24.04
flatpak-builder --user --force-clean --install build org.sugarlabs.TypingTurtle.json
```

## Check For Updates

Install the flatpak external data checker
```
flatpak --user install org.flathub.flatpak-external-data-checker
```

Now to update every single module to the latest stable version use
```
cd org.sugarlabs.TypingTurtle
flatpak run --filesystem=$PWD org.flathub.flatpak-external-data-checker org.sugarlabs.TypingTurtle.json
```
