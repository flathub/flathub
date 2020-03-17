# Flatpak for nomacs

![Screenshot](https://raw.githubusercontent.com/egrath/nomacs-flatpak/master/nomacs_screenshot_01.jpg)

**Build it**
```
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
```

```
flatpak-builder --force-clean --install-deps-from=flathub --keep-build-dirs build com.github.nomacs.json
```

**Test it**
```
flatpak-builder --run  build com.github.nomacs.json sh
nomacs
```

**Export to repository**
```
flatpak-builder --repo=repo --force-clean build com.github.nomacs.json
```

**Add repository**
```
flatpak remote-add --no-gpg-verify --user nomacs-repository repo
```

**Install from repository**
```
flatpak install --user --assumeyes nomacs-repository com.github.nomacs
```
... or use the software center if you prefer the graphical approach

**Run nomacs**
You're all set:
```
flatpak run com.github.nomacs
```

**Optionally, if you want to create a .flatpak**
```
flatpak build-bundle repo nomacs312.flatpak com.github.nomacs 3.12
```

