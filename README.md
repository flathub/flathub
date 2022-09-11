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
- Documents folder (to load and save JMX files)

## Known issues

Since SoapUI is using the root of the home folder to store the configuration files `soapui-settings.xml` and `default-soapui-workspace.xml`, in this package it has been proposed to use `~/.var/app/com.eviware.soapui/config/` as its home folder, so that in case the user selects to share his home folder it don't save those files in its root (examples below).

The application has been designed to run using OpenJDK16 but currently this Flatpak has been configured to use [OpenJDK17](https://github.com/flathub/org.freedesktop.Sdk.Extension.openjdk17).

Also currently accessing to external web pages is not working due to this [external issue](https://github.com/flathub/org.freedesktop.Sdk.Extension.openjdk17/issues/1).

## Special configurations and workarrounds

Since the application folder is readonly it has been enabled the following paths have been set:

- User properties: `~/.var/app/com.eviware.soapui/config/soapui.properties`
- User external actions: `~/.var/app/com.eviware.soapui/data/actions`
- User external extensions: `~/.var/app/com.eviware.soapui/data/ext`
- User external libraries: `~/.var/app/com.eviware.soapui/data/listeners`
- SoapUI user home directory: `~/.var/app/com.eviware.soapui/config/`
  - User settings: `~/.var/app/com.eviware.soapui/config/soapui-settings.xml`
  - User workspace: `~/.var/app/com.eviware.soapui/config/default-soapui-workspace.xml`
  - User plugins: `~/.var/app/com.eviware.soapui/config/.soapuios/plugins/`
  - Logs: `~/.var/app/com.eviware.soapui/config/.soapuios/logs/`

This variables has been set by coping the functionality of the `soapui.sh` into the `soapui-launcher.sh` (in this repository) since the former don't allow us to change the `JAVA_OPTS`.
