# [Digitales Register](https://github.com/Tobias-Bucci/digitales_register) Flatpak

This is the inofficial client for the Digitales Register used in schools in Südtirol, for more information refer to the app's documentation

## Known issues

- Opening attachments doesn't do anything

## Development

Edit the `flatpak-flutter.yml` not the `io.github.tobias_bucci.digitales_register.yml`

### To generate the manifest

1. Get and setup the [flatpak-flutter](https://github.com/TheAppgineer/flatpak-flutter) generator
2. Run `flatpak-flutter.py --app-module digitales_register flatpak-flutter.yml` after editing `flatpak-flutter.yml`
3. Continue as normal with `flatpak-builder`
