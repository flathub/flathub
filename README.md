# The Ipe extensible drawing editor

Official website: https://ipe.otfried.org/

Ipe is a drawing editor for creating figures in PDF format. It supports making small figures for inclusion into LaTeX-documents as well as making multi-page PDF presentations.

## Using TeX Live with the SDK (recommended)

To use LaTeX to typeset labels in Ipe, an installation of TeX Live is needed. One way this can be provided is through [org.freedesktop.Sdk.Extension.texlive](https://github.com/flathub/org.freedesktop.Sdk.Extension.texlive/) which is natively supported by this Flatpak. Just install the latest version of the SDK and it is automatically detected and used.

## Using an existing TeX Live installation (not recommended)

Sometimes it is preferred to use an existing upstream TeX Live installation. In this case an existing path can be mounted into the sandbox (given that it doesn't already exist inside) and Ipe made aware of where to find it.

```
flatpak run --filesystem=/opt/texlive/2023:ro --env=IPELATEXPATH=/opt/texlive/2023/bin/x86_64-linux org.otfried.Ipe
```

or use `override` to make it permanent

```
flatpak override --filesystem=/opt/texlive/2023:ro --env=IPELATEXPATH=/opt/texlive/2023/bin/x86_64-linux org.otfried.Ipe
```
