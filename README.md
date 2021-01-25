# org.nitrokey.app - Nitrokey App Flatpak

This is the (likely temporary) flatpak build repository for the Nitrokey App.

*Temporary*, as we might distribute the flatpak via [flathub](https://flathub.com), but
this will for sure not be instant, see: [Flathub App Submission](https://github.com/flathub/flathub/wiki/App-Submission).

To build *and* install it locally, just use:
```
make
```

If you would like to just build, please run:
```
make build
```

... ensure `flatpak` and `flatpak-build` is available on your system.

To finally run, just use:
```
make run
```
or via flatpak:
```
flatpak run org.nitrokey.app
```

