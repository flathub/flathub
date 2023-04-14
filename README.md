# Flatpak packaging for Supersonic

[Supersonic](https://github.com/dweymouth/supersonic/) is a desktop music players that works with
Subsonic-compatible servers like [Navidrome](https://www.navidrome.org/about/).

To build this locally, try:

    flatpak-builder build-dir com.github.dweymouth.supersonic.yml

Then to install:

    flatpak-builder --user --install --force-clean build-dir com.github.dweymouth.supersonic.yml

And run:

    flatpak run com.github.dweymouth.supersonic

You will need `flatpak` and `flatpak-builder` packages for the above
to work.

Once this is [published on Flathub](https://discourse.flathub.org/t/supersonic-lightweight-cross-platform-desktop-client-for-subsonic-music-servers/3984/), only `flatpak` will be
required for running, of course.

## Regenerating sources files

To regenerate the modules list, run this against your supersonic
source tree (e.g. `~/dist/supersonic` below):

    go run github.com/dennwc/flatpak-go-mod@latest ~/dist/supersonic

### Digression

It's surprisingly difficult to get this to work right. First off, it's
nearly impossible to come up with a full list of dependencies by hand,
only animals and Debian developers do something like this. (To be
fair, there's a good reason Debian developer do that, but it does not
make sense in the Flatpak context in any case.)

There are various tools in various state of brokenness out there that
allow you to automatically generate a list of sources entry. I have
tried them all, let me give you a tour.

 * [flatpak-builder-tools](https://github.com/flatpak/flatpak-builder-tools/) has a [go-get](https://github.com/flatpak/flatpak-builder-tools/tree/master/go-get) directory with two
   tools there: [flatpak-go-get-generator.py](https://github.com/flatpak/flatpak-builder-tools/blob/master/go-get/flatpak-go-get-generator.py) and
   [flatpak-go-vendor-generator.py](https://github.com/flatpak/flatpak-builder-tools/blob/master/go-get/flatpak-go-vendor-generator.py). The former is typically what
   gets recommended (and is used by [syncthing](https://github.com/flathub/me.kozec.syncthingtk/blob/cf25336132b96c514f29d5a5322874e847aba150/syncthing.yaml)), but it didn't
   work for my use case, as it was missing some dependencies. The
   latter *did* generate a proper list, but it was slow as hell and it
   ended up not compiling anyway because of obscure golang
   reasons. Both of those generate links to git repositories which are
   atrociously slow and do not cache well, so I moved on.

 * the game [aaaaxy](https://github.com/divVerent/aaaaxy) has a [go-vendor-to-flatpak-yml.sh](https://github.com/divVerent/aaaaxy/blob/main/scripts/go-vendor-to-flatpak-yml.sh) shell
   script which may work, but it's long enough that I got scared and
   promptly moved away

 * [com.yktoo.ymuse]( https://github.com/flathub/com.yktoo.ymuse) has a [clever script](https://github.com/flathub/com.yktoo.ymuse/blob/7c33add48cfc6788b9ddf0c92ee7213f40c6c78e/com.yktoo.ymuse.yml#L57-L63) that parses the
   output of `go download -json` (but without treating it as JSON) and
   generates synthetic URLs to the Golang proxy. The problem is the
   generated archives have the version number in them which makes
   Golang unhappy. The ymuse manifest has a hack to rename those
   folders but that seems kind of awful and brittle so I have also
   moved on.

I ended up using [dennwc](https://github.com/dennwc/flatpak-go-mod/) and i audited [this version of the
source](https://github.com/dennwc/flatpak-go-mod/blob/af6ec8b977f3ba97b8d8f0b02243326111ff32a1/main.go). It "Just Works".
