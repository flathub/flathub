<img align="left" style="vertical-align: middle" width="120" height="120" src="src/icons/io.gitlab.elescoute.spacelaunch.png">

# Space Launch

Space Launch is a native unofficial SpaceLaunchNow.me client for GNOME, written in Vala and GTK4. It allows to keep track of upcoming rocket launches.

### 

[![Please do not theme this app](https://stopthemingmy.app/badge.svg)](https://stopthemingmy.app)
[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](http://www.gnu.org/licenses/gpl-3.0)

![Upcoming launches](./data/appdata/screenshot01.png)
![Details of launch](./data/appdata/screenshot05.png)

Space Launch is currently in alpha release. Some crashes due to problems of communication with spacelaunchnow.me server can remain. More features will be added later.

## Features

* Lists all upcoming launches.
* Displays details of each launch (Mission, orbit, status, countdown,...).
* Redirect to video stream if available.

## Install from Flathub

<a href="https://flathub.org/apps/details/io.gitlab.elescoute.spacelaunch">
<img src="https://flathub.org/assets/badges/flathub-badge-en.png" width="120"/></a>

## Dependencies

Please make sure you have these dependencies first before building.

```
gtk4
libadwaita-1
meson
vala
```

## How to build

Simply clone this repo, then:

```
meson _build --prefix=/usr && cd _build
sudo ninja install
```

## Change Log

Check the [release tags](https://gitlab.com/elescoute/spacelaunch/-/tags) for change log.
