# grapejuice-flatpak
## Installation
### Step 1
Download the GJTestRepo locally by running the following commands
```bash
git clone https://github.com/Thelolguy1/grapejuice-flatpak.git
cd grapejuice-flatpak
```
### Step 2
Add the flathub and the GJTestRepo
```bash
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak --user remote-add --no-gpg-verify grapejuiceTest GJTestRepo
```
### Step 3
Finally install the grapejuice flatpak
```bash
flatpak --user install grapejuiceTest com.gitlab.brinkervii.grapejuice
```
_If you want a global installation remove the '--user' flag from Step 2/3_

## Simplified installation
You can run ./install.sh and ./uninstall.sh for an automated install

## Build It Yourself
To build the flatpak by yourself, run
1. build.sh (generates build-dir)
2. run_bash.sh and verify that the container works.
3. export.sh (generates GJTestRepo)

## Disclaimer
The wine builds stored in wine_builds are repackaged from community-sourced builds, as retrieved from (https://brinkervii.gitlab.io/grapejuice/docs/Guides/Installing-Wine.html). I am not liable for any damages caused by the usage of the builds.

## Honorable Mentions
Thank you to Infinitybeond1, LithRakoon, Soikr, z-ffqq, and others for testing and development.
