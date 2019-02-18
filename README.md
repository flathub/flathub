# SDK Extension for OpenJDK 12

This extension contains the OpenJDK 12 Java Runtime Environment (JRE) and Java Developement Kit (JDK).

OpenJDK 12 is the current latest (non-LTS) version.

For the current long-term support (LTS) version, see the [OpenJDK 11](https://github.com/flathub/org.freedesktop.Sdk.Extension.openjdk11) extension.

## Usage

You can bundle the JRE with your Flatpak application by adding this SDK extension to your Flatpak manifest and calling the install.sh script. For example:

```
{
  "id" : "org.example.MyApp",
  "branch" : "1.0",
  "runtime" : "org.freedesktop.Platform",
  "runtime-version" : "18.08",
  "sdk" : "org.freedesktop.Sdk",
  "sdk-extensions" : [ "org.freedesktop.Sdk.Extension.openjdk" ],
  "modules" : [ {
    "name" : "openjdk",
    "buildsystem" : "simple",
    "build-commands" : [ "/usr/lib/sdk/openjdk/install.sh" ]
  }, {
    "name" : "myapp",
    "buildsystem" : "simple",
    ....
  } ]
  ....
  "finish-args" : [ "--env=PATH=/app/jre/bin:/usr/bin" ]
}
```
