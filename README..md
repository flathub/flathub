## Flatpak
The TUMexam client can be build and installed using Flatpak.

### Requirements
#### Fedora
```
sudo dnf install flatpak flatpak-builder
flatpak install flathub org.gnome.Sdk//42 org.gnome.Platform//42
```

#### Debian/Ubuntu
```
sudo apt install flatpak flatpak-builder
flatpak install flathub org.gnome.Sdk//42 org.gnome.Platform//42
```

### Building
```
git clone <THIS_REPOSITORY>
cd <CLONED_REPOSITORY>
flatpak-builder --force-clean flatpak_build_dir de.tumexam.cli.yml
```

### Installing
```
flatpak-builder --user --install --force-clean flatpak_build_dir de.tumexam.cli.yml
```

### Uninstalling
```
flatpak uninstall de.tumexam.cli
```

### Executing
```
flatpak run de.tumexam.cli
```