# SDK Extension for OpenJDK 17

This extension contains the OpenJDK 17 Java Runtime Environment (JRE) and Java Developement Kit (JDK).

OpenJDK 17 is the current long-term support (LTS) version.

For the previous LTS version, see the [OpenJDK 11](https://github.com/flathub/org.freedesktop.Sdk.Extension.openjdk11) extension.

For the current latest (non-LTS) version, see the [OpenJDK](https://github.com/flathub/org.freedesktop.Sdk.Extension.openjdk) extension.

## Usage

You can bundle the JRE with your Flatpak application by adding this SDK extension to your Flatpak manifest and calling the install.sh script. For example:

```
{
  "id" : "org.example.MyApp",
  "branch" : "1.0",
  "runtime" : "org.freedesktop.Platform",
  "runtime-version" : "21.08",
  "sdk" : "org.freedesktop.Sdk",
  "sdk-extensions" : [ "org.freedesktop.Sdk.Extension.openjdk17" ],
  "modules" : [ {
    "name" : "openjdk",
    "buildsystem" : "simple",
    "build-commands" : [ "/usr/lib/sdk/openjdk17/install.sh" ]
  }, {
    "name" : "myapp",
    "buildsystem" : "simple",
    ....
  } ]
  ....
  "finish-args" : [ "--env=PATH=/app/jre/bin:/usr/bin" ]
}
```
