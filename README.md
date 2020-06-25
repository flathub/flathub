com.parsecgaming.parsec flatpak
===

This flatpak is a wrapper for the [Parsec cloud gaming client](https://parsecgaming.com/download).

It's currently experimental, feel free to use and contribute yourself by building it on your machine.

Building
---

Install `flatpak-builder` and make sure your local flatpak setup contains the `org.freedesktop.Sdk` as well as the `org.freedesktop.Platform`.

To build the actual parsec flatpak use:

```
git clone https://github.com/flathub/parsec-flatpak.git
cd parsec-flatpak
flatpak-builder builddir com.parsecgaming.parsec.yml --force-clean
```

If you want to install it immediately to be ready to run it locally, you can use the following command instead:

```
flatpak-builder builddir com.parsecgaming.parsec.yml --force-clean --install --user
```

TODOs
---

- [x] Provide a wrapper for the parsec `.deb`
- [x] Provide a `.desktop` file for the flatpak
- [x] Provide basic information for appdata
- [x] Provide screenshots for appdata
- [x] Rework the flatpak to use extra-data for the `.deb`
- [ ] Contact Parsec Cloud Inc about proper versioning
- [ ] Apply for flathub submission
