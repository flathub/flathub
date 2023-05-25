# com.github.Pyfa

Pyfa (Python fitting assistant for EVE: Online)

https://github.com/pyfa-org/Pyfa

## How to build Pyfa

```
flatpak-builder --repo=repo flatpakbuildir com.github.Pyfa.json --force-clean
```

## Add Pyfa repo to remote

```
flatpak remote-add --user --no-enumerate --no-gpg-verify mypyfa repo
```

## How to install Pyfa from flatpak

```
flatpak install --user mypyfa com.github.Pyfa
```

## How to run Pyfa

```
flatpak run com.github.Pyfa
```

## Use existing profile

Launch Pyfa and close it to create profile directory structure. Then copy existing profile to flatpak app dir
```
cp -a ~/.pyfa/. ~/.var/app/com.github.Pyfa/data/pyfa
```

## How to uninstall Pyfa

```
flatpak uninstall --user com.github.Pyfa
```


