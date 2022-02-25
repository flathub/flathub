# PyQt BaseApp

This base application is designed to be used for packaging Flatpak applications that use PyQt,  
Riverbank Computing's Python bindings for [The Qt Company's](https://www.qt.io/) Qt application framework.

**Note: This packaging is not affilated with, or supported by Riverbank Computing Limited.**

## What's included?

* [QtWebEngine base application](https://github.com/flathub/io.qt.qtwebengine.BaseApp)
* [PyQt](https://riverbankcomputing.com/software/pyqt/) Python bindings for Qt
* [PyQtWebEngine](https://riverbankcomputing.com/software/pyqtwebengine) Python bindings for QtWebEngine
* Run-time dependencies for the above, including:
  * krb5
  * libevent

### Build tools and their dependencies

Build tools are included to help package extra PytQt bindings and Python modules. These tools are removed by the  
`BaseApp-cleanup.sh` script.

#### Python module build tools

* python-build
* python-flit-core
* python-installer
* python-packaging
* python-pep517
* python-setuptools-scm

#### PyQt build tools

* pyqt-builder
* pyqt-sip
* sip

#### Retained Python modules

While the following Python modules are dependencies of the mentioned build tools, they are not removed by the  
`BaseApp-cleanup.sh` script, as they might be needed by the Flatpak application.

* python-pyparsing
* python-tomli
* python-toml

## How to use?

### Example: PyQtWebEngine application

```yaml
app-id: org.kde.PyQtWebEngineApp
runtime: org.kde.Platform
runtime-version: 5.15-21.08
sdk: org.kde.Sdk
base: com.riverbankcomputing.PyQt.BaseApp
base-version: 5.15-21.08
cleanup-commands:
  - /app/cleanup-BaseApp.sh
modules:
  - name: PyQtWebEngineApp
...
```

### Example: PyQt application

While it's not required, it's possible to set the environment variable `BASEAPP_REMOVE_PYWEBENGINE` to have the  
`BaseApp-cleanup.sh` script remove the PyQtWebEngine bindings and QtWebEngine with its dependencies.

```yaml
app-id: org.kde.PyQtApp
runtime: org.kde.Platform
runtime-version: 5.15-21.08
sdk: org.kde.Sdk
base: com.riverbankcomputing.PyQt.BaseApp
base-version: 5.15-21.08
cleanup-commands:
  - /app/cleanup-BaseApp.sh
build-options:
  env:
    - BASEAPP_REMOVE_PYWEBENGINE=1
modules:
  - name: PyQtApp
...
```
