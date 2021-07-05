Roll 'm Up
===

This is Flatpak package for Roll 'm Up, classic pinball game. 
published in Flathub.org: https://flathub.org/apps/details/nl.lostboys.rollemup. 

Roll 'm Up is abandoned proprietary software created by Lostboys Media Lab 
for Dommelsch Brewery from Netherlands in 1998.

Known issues
---

- Highscore upload feature does not work: external servers are gone
  for many years. You can no longer win beer in this game.
- Resizing window sometimes causes the game to crash. 
  Please, don't resize it. Content scaling doesn't work anyway.
- It's impossible to use non-ASCII characters in highscore nickname.
  Default "Player 1" nickname is fine.
- Application always crashes when closing. We can live with that.

How to install?
---

1. Make sure you installed Flatpak and Flathub.org repository: 
   https://flatpak.org/setup/.
2. Search for `Roll 'm Up` in GNOME Software or KDE Discover.
   You can also use this command in terminal:
   `flatpak install nl.lostboys.rollemup`.
3. Click `Roll 'm Up` icon in your desktop launcher or run this command:
   `flatpak run nl.lostboys.rollemup`.

How to build?
---

Make sure you have Flatpak and `flatpak-builder` installed. 

Install Flathub.org repository user-wide:

```bash
flatpak remote-add flathub https://flathub.org/repo/flathub.flatpakrepo \
   --if-not-exists  --user
```

Use this command to build the package and install it locally:

```bash
flatpak-builder build nl.lostboys.rollemup.yaml \
   --force-clean --install-deps-from=flathub --user --install
```

Resources
---

- https://aur.archlinux.org/packages/rollemup/
- https://www.pro-linux.de/artikel/2/1663/roll39m-up-ein-altes-flipperspiel-neu-entdeckt.html
- https://web.archive.org/web/20080613193931/https://www.jmurray.id.au/rollem.html
