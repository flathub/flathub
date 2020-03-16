# Nomacs Image viewer Flatpak build

![Screenshot](https://raw.githubusercontent.com/egrath/nomacs-flatpak/master/nomacs_screenshot_01.jpg)

**Build it**
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

**Optionally, if you want to create a .flatpak**
```
flatpak build-bundle repo nomacs312.flatpak com.github.nomacs 3.12
```

