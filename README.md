# com.jetbrains.PyCharm-Professional

PyCharm Professional Edition IDE editor for developing not only python applications.

You only need to download sources from www.jetbrains.com/pycharm

## How to build PyCharm-Professional

```
flatpak-builder --repo=repo build com.jetbrains.PyCharm-Professional.yaml --force-clean
```

## Add PyCharm-Professional repo to remote

```
flatpak remote-add --user mypycharm repo
```

## How to install PyCharm-Professional from flatpak

```
flatpak install --user mypycharm com.jetbrains.PyCharm-Professional
```

## How to run PyCharm-Professional

```
flatpak run com.jetbrains.PyCharm-Professional
```

## How tu uninstall PyCharm-Professional

```
flatpak uninstall --user com.jetbrains.PyCharm-Professional
```
