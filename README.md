# SDK Extension for OpenJFX 21

This extension contains the OpenJFX 21 jmods.

OpenJFX 21 is the current latest version. This is *not* a long-term support (LTS) version and will be periodically updated as new JFX are released.

## Usage

You can create your own JavaFX custom runtime images (JRE) for your Flatpak application by adding this SDK extension to your Flatpak manifest along with an openjdk extension. For example:

```
id: org.example.MyApp
runtime: org.freedesktop.Platform
runtime-version: '23.08'
sdk: org.freedesktop.Sdk
sdk-extensions:
  - org.freedesktop.Sdk.Extension.openjdk
  - org.freedesktop.Sdk.Extension.openjfx
separate-locales: false

modules:
  - name: cri
    buildsystem: simple
    build-commands:
      - /usr/lib/sdk/openjdk/bin/jlink 
        --verbose 
        --ignore-signing-information 
        --no-header-files 
        --no-man-pages 
        --strip-debug 
        --compress zip-9 
        --module-path /usr/lib/sdk/openjfx/jmods 
        --add-modules javafx.controls
        --output /app/cri
    build-options:
      no-debuginfo: true

  - name: the_app
    buildsystem: simple
    build-commands:
```
