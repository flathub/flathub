# Apache JMeter for Flatpak

Unofficial Apache JMeter Flatpak package.

Currently waiting for [official response](https://bz.apache.org/bugzilla/show_bug.cgi?id=66258) as is recommended by the [Flathub App Submission](https://github.com/flathub/flathub/wiki/App-Submission) page.

## Permissions

- GUI: x11,ipc,dri
- Network (to allow perfonming the actual tests)
- Documents folder (to load and save JMX files)

## Special configurations

Since the application folder is readonly it has been enabled the following configurations and paths have been changed:

- User configuration: `~/.var/app/org.apache.jmeter/config/jmeter/user.properties`
- User plugins directory: `~/.var/app/org.apache.jmeter/data/jmeter/lib/ext/`
- Application log: `~/.var/app/org.apache.jmeter/.local/state/jmeter/jmeter.log`
- Force use local help browser to ease accessing the help while this [external issue](https://github.com/flathub/org.freedesktop.Sdk.Extension.openjdk17/issues/1) is not fixed.

## Known issues

Currently **JavaFX** is not installed. Not sure yet what is the best, if add it manually as a module or as a runtime extension (as is proposed in [this issue](https://github.com/flathub/org.freedesktop.Sdk.Extension.openjdk/issues/1)).

Also the current configuration doesn't allow the **[JMeter Plugins Manager](https://github.com/undera/jmeter-plugins-manager)** to install plugins, this can be fixed modifying the plugin and [an issue](https://groups.google.com/g/jmeter-plugins/c/-rJeqHz8lCw) have been created to allow custom property to be used to designate the JMeter home to be used so that the `jmeter-wrapper.sh` can point it to `~/.var/app/org.apache.jmeter/data/jmeter/`.
