thunderbird-flatpak
===================

Resources to build Mozilla Thunderbird as a flatpak.

Requirements:
------------

  * flatpak >= 0.6.6
  * flatpak-builder >= 0.6.6
  * org.gnome Platform and Sdk runtimes

Instructions:
-------------

(1) Install the flatpak repository for GNOME nightly:
```
  wget http://sdk.gnome.org/nightly/keys/nightly.gpg
  flatpak --user remote-add --gpg-import=nightly.gpg gnome-nightly http://sdk.gnome.org/nightly/repo
```
(2) Install the required runtimes
```
  flatpak --user install gnome-nightly org.gnome.Platform
  flatpak --user install gnome-nightly org.gnome.Sdk
```
(3) Build thunderbird from this directory:
```
  flatpak-builder --force-clean --ccache --require-changes \
      --repo=repo app \
      org.mozilla.Thunderbird.json
```
(4) Add a remote to your local repo and install it:
```
  flatpak --user remote-add --no-gpg-verify thunderbird-repo /path/to/your/flatpak/repo
  flatpak --user install thunderbird-repo org.mozilla.Thunderbird
```
(5) Run thunderbird as an flatpak:
```
  flatpak run org.mozilla.Thunderbird
```

Note that if you do further changes in the `appdir` (e.g. to the metadata), you'll need to re-publish it in your local repo and update before running it again:
```
  flatpak build-export /path/to/your/flatpak/repo /path/to/flatpak/appdir
  flatpak --user update org.mozilla.Thunderbird
```

Last, you can bundle chromium to a file with the `build-bundle` subcommand:
```
  flatpak build-bundle /path/to/your/flatpak/repo thunderbird.bundle org.mozilla.Thunderbird
```

Prebuilt flatpaks
-----------------

By popular request, I've built myself a flatpak bundle of Thunderbird against the org.gnome.Platform runtime, for x86_64/3.24.

Grab them from here:
  * [Mozilla Thunderbird for org.gnome.Platform/x86_64/3.24](https://raw.githubusercontent.com/mariospr/thunderbird-flatpak/master/bundles/52.1/x86_64/3.24/org.mozilla.Thunderbird.flatpak)
