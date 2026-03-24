# SDK Extension for Qt 6.11

This extension contains the [Qt](https://www.qt.io/) 6.11 libraries and development tools, providing a comprehensive cross-platform application framework for building modern desktop and embedded applications.

Included modules: QtBase, QtDeclarative (QML/Quick), QtWayland, QtMultimedia, QtSvg, QtImageFormats, QtTools, QtShaderTools, Qt5Compat, QtWebSockets, QtWebChannel, QtPositioning, QtLocation, QtConnectivity, QtSerialPort, QtSerialBus, QtSensors, QtSCXML, QtRemoteObjects, QtCharts, QtNetworkAuth, QtHttpServer, QtQuickTimeline, QtQuick3D, QtQuick3DPhysics, QtGrpc, QtSpeech, QtVirtualKeyboard, QtLottie, Qt3D, QtGraphs, and more.

**QtWebEngine is NOT included** due to its extreme build size (Chromium). Use a separate extension or the system WebEngine if needed.

## Usage

### Build

In order to build your app with Qt 6.11 provided by this extension you have to set the following variables in your app manifest:

```json
{
  "sdk-extensions": ["org.freedesktop.Sdk.Extension.qt611"],
  "build-options": {
    "append-path": "/usr/lib/sdk/qt611/bin",
    "prepend-ld-library-path": "/usr/lib/sdk/qt611/lib",
    "env": {
      "QTDIR": "/usr/lib/sdk/qt611",
      "QT_PLUGIN_PATH": "/usr/lib/sdk/qt611/lib/plugins"
    }
  }
}
```

Example:
```json
{
  "id": "org.example.MyApp",
  "runtime": "org.freedesktop.Platform",
  "runtime-version": "25.08",
  "sdk": "org.freedesktop.Sdk",
  "sdk-extensions": ["org.freedesktop.Sdk.Extension.qt611"],
  "modules": [
    {
      "name": "MyApp",
      "buildsystem": "cmake-ninja",
      "build-options": {
        "append-path": "/usr/lib/sdk/qt611/bin",
        "prepend-ld-library-path": "/usr/lib/sdk/qt611/lib",
        "env": {
          "QTDIR": "/usr/lib/sdk/qt611",
          "QT_PLUGIN_PATH": "/usr/lib/sdk/qt611/lib/plugins"
        }
      }
    }
  ]
}
```

### Debugging/Development

In order to use this extension in a Flatpak SDK environment you may add all provided tools to your PATH by executing:
```
source /usr/lib/sdk/qt611/enable.sh
```
