# Currency Converter
A linux app that simplifies currency conversions. The users can input any currency value, and instantly see its equivalent in another currency. It's straightforward, fast, and easy to use.

## Getting Started
installation instructions:
```
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install flathub com.blunix.currency_converter
flatpak run com.blunix.currency_converter
```
build instructions:
```
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.freedesktop.Platform//23.08 org.freedesktop.Sdk//23.08
flatpak install --user flathub org.flatpak.Builder
flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/media/ --repo=repo builddir com.blunix.currency_converter.yml
flatpak run com.blunix.currency_converter
```
## Developer Information
Developed and published by:
```
Blunix GmbH
Glogauer Straße 21
10999 Berlin
Germany
```

Web: https://www.blunix.com

For inqueries please contact: info@blunix.com

Lead Developer: Nikoloz Miruashvili