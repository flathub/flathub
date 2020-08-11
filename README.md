# com.jetbrains.GoLand

GoLand is a Capable and Ergonomic Go IDE

You only need to download sources from www.jetbrains.com/go

## How to build GoLand

```
flatpak-builder --repo=repo build com.jetbrains.GoLand.yaml --force-clean
```

## Add GoLand repo to remote

```
flatpak remote-add --user mygoland repo
```

## How to install GoLand from flatpak

```
flatpak install --user mygoland com.jetbrains.GoLand
```

## How to run GoLand

```
flatpak run com.jetbrains.GoLand
```

## How tu uninstall GoLand

```
flatpak uninstall --user com.jetbrains.GoLand
```
