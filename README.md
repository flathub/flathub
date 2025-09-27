# SDK Extension for OpenJDK 25

This extension contains the OpenJDK 25 Java Runtime Environment (JRE) and Java Developement Kit (JDK).

OpenJDK 25 is the current long-term support (LTS) version.

For the previous LTS version, see the [OpenJDK 21](https://github.com/flathub/org.freedesktop.Sdk.Extension.openjdk21) extension.

For the current latest (non-LTS) version, see the [OpenJDK](https://github.com/flathub/org.freedesktop.Sdk.Extension.openjdk) extension.

## Usage

You can bundle the JRE with your Flatpak application by adding this SDK extension to your Flatpak manifest and calling the install.sh script. For example:

```
app-id: com.example.myapp
runtime: org.freedesktop.Platform
runtime-version: '25.08'
sdk: org.freedesktop.Sdk
sdk-extensions:
  - org.freedesktop.Sdk.Extension.openjdk25
command: myapp

finish-args:
  - --socket=x11
  - --share=ipc
  - --env=PATH=/app/jre/bin:/app/bin:/usr/bin
  - --env=JAVA_HOME=/app/jre
  # ...

modules:
  - name: openjdk
    buildsystem: simple
    build-commands:
      - /usr/lib/sdk/openjdk25/install.sh

  - name: myapp
    buildsystem: simple
    build-options:
      env:
        PATH: /app/bin:/usr/bin:/usr/lib/sdk/openjdk25/bin
        JAVA_HOME: /usr/lib/sdk/openjdk25/jvm/openjdk-25
    build-commands:
      - install -Dm755 -t /app/bin myapp
      - install -Dm644 -t /app/share/com.example.myapp myapp.jar
      # ...
    sources:
      - type: archive
        url: https://example.com/myapp/download/myapp-1.0.0.tar.gz
        # ...
      - type: script
        dest-filename: myapp
        commands:
          - exec java -jar /app/share/com.example.myapp/myapp.jar $@
      # ...
```
