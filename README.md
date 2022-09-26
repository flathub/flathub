# SoapUI OS for Flatpak

Unofficial SoapUI Open Source Flatpak package.

Currently waiting for [official response](https://github.com/SmartBear/soapui/issues/744) as is recommended by the [Flathub App Submission](https://github.com/flathub/flathub/wiki/App-Submission) page.

## Application Id concers

For now we are not sure yet what Application Id is prefered by SmartBear Software since they are using 2 Ids for this aplication:

- `com.eviware.soapui`
- `com.smartbear.soapui`

Or maybe even to append the `-os` (opensource version) in any of the above or whatever they want.

## Permissions

- GUI: x11,ipc,dri
- Network (to allow perfonming the actual tests)
- Documents folder (to load and save project files)

## Known issues

Currently accessing to external web pages is not working due to this [external issue](https://github.com/flathub/org.freedesktop.Sdk.Extension.openjdk17/issues/1).

## Special configurations and workarrounds

Since the application folder is readonly it has been enabled the following paths have been set:

- User external actions: `~/.var/app/com.eviware.soapui/.soapuios/actions`
- User external extensions: `~/.var/app/com.eviware.soapui/.soapuios/ext`
- User external libraries: `~/.var/app/com.eviware.soapui/.soapuios/listeners`
- User plugins: `~/.var/app/com.eviware.soapui/.soapuios/plugins/`
- User properties: `~/.var/app/com.eviware.soapui/.soapuios/soapui.properties`

This variables has been set by coping the functionality of the `soapui.sh` into the `soapui-launcher.sh` (in this repository) since the former don't allow us to change the `JAVA_OPTS`.

Also since SoapUI is using the root of the home folder to store some configuration files (`soapui-settings.xml` and `default-soapui-workspace.xml`), in the launcher it symlinks them (if they don't already exist) into the `.soapuios` folder so that the settings can be persisted.
