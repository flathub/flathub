# org.ioquake3.IOQuake3

Flatpak recipe for <http://ioquake3.org>.

## Install

```shell
$ flatpak-builder build --install --user ./org.ioquake3.IOQuake3.yaml
```

## Flathub?

Definitely!

I just need to:
 - … [add an appstream file][1]
 - … [convince upstream to make releases][2]
 - … [ensure I don't keep any crap around][3]
 - … add a flathub.json file (I believe).

[1]: https://github.com/flathub/flathub/wiki/App-Requirements#appstream
[2]: https://github.com/flathub/flathub/wiki/App-Requirements#stable-releases-reproducible-builds
[3]: https://github.com/flathub/flathub/wiki/App-Requirements#bundled-dependencies
