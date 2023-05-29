# org.mnemosyneproj.Mnemosyne
## Installation for testing purposes

* Make sure flatpak and flatpak-builder is installed.
For Ubuntu run:
```
sudo apt-get install flatpak flatpak-builder
```
Now the flatpak bits can be run.
* installing the flathub repository for dependencies
```
$ flatpak remote-add --if-not-exists --user flathub https://flathub.org/repo/flathub.flatpakrepo
```
* cloning the git and building with flatpak-builder
```
$ git clone https://github.com/linuxrider/org.mnemosyneproj.Mnemosyne.git
$ cd org.mnemosyneproj.Mnemosyne/
$ flatpak-builder build --force-clean --install-deps-from=flathub --install --user org.mnemosyneproj.Mnemosyne.yaml
```
for testing purposes --ccache option is beneficial because it reduces compilation time significantly for subsequent runs
* uninstalling can be done easily too
```
$ flatpak uninstall --delete-data --user org.mnemosyneproj.Mnemosyne
```
* if you want to remove also build folder and dependecies do this
```
$ rm -rf .flatpak-builder/ build/
$ flatpak uninstall --unused --user
$ flatpak remote-delete --user flathub
```
## Configuration
Configuration resides in `~/var/app/org.mnemosyneproj.Mnemosyne/`
