# com.github.pyfa_org.Pyfa

Pyfa (Python fitting assistant for EVE: Online)

https://github.com/pyfa-org/Pyfa

## How to get flatpak build manifest

```
git clone https://github.com/asbytes/com.github.pyfa_org.Pyfa.git
cd com.github.pyfa_org.Pyfa
git submodule update --init
```

## How to build Pyfa

```
flatpak-builder --repo=repo flatpakbuildir com.github.pyfa_org.Pyfa.json --force-clean
```

## Add Pyfa repo to remote

```
flatpak remote-add --user --no-enumerate --no-gpg-verify mypyfa repo
```

## How to install Pyfa from flatpak

```
flatpak install --user mypyfa com.github.pyfa_org.Pyfa
```

## How to run Pyfa

```
flatpak run com.github.pyfa_org.Pyfa
```

## Use existing profile

Launch Pyfa and close it to create profile directory structure. Then copy existing profile to flatpak app dir
```
cp -a ~/.pyfa/. ~/.var/app/com.github.pyfa_org.Pyfa/data/pyfa
```

## How to uninstall Pyfa

```
flatpak uninstall --user com.github.pyfa_org.Pyfa
```


