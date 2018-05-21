HOW TO BUILD MIKUTTER FLATPAK
=============================
PREREQUISITE
------------
flatpak-builder

[Quick setup guide](https://flatpak.org/setup/)

### Update version number in config
Edit 2 files.

1. `net.hachune.mikutter.mikutter.git-source.json`  
   Update `"tag"` field.
2. `net.hachune.mikutter.mikutter.appdata.xml`  
   Prepend new `<release/>` element to `<releases>`.

### Update RubyGems dependencies
```bash
# pwd is repository root
git checkout X.Y.Z # checkout correct revision
rm -rf vendor/cache
bundle install
bundle package # cache gems in vendor/cache
ruby path/to/flatpak_rubygems_generator.rb --source net.hachune.mikutter.mikutter.git-source.json -o net.hachune.mikutter.mikutter.rubygems-module.json # update json
```

BUILD
-----
```bash
# pwd is deployment/flatpak
flatpak install flathub org.gnome.Platform//3.28 org.gnome.Sdk//3.28
flatpak-builder build-dir net.hachune.mikutter.mikutter.json
# to rebuild
flatpak-builder --force-clean build-dir net.hachune.mikutter.mikutter.json
```

TEST
----
```bash
flatpak-builder --run build-dir net.hachune.mikutter.mikutter.json mikutter
# setting up repo and run in **production environment**
flatpak-builder --repo=local-repo-dir --force-clean build-dir net.hachune.mikutter.mikutter.json
flatpak --user remote-add --no-gpg-verify local-repo local-repo-dir
flatpak --user install local-repo net.hachune.mikutter.mikutter
flatpak run net.hachune.mikutter.mikutter
```
