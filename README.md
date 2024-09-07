# Cell Game Activity Flatpak

Aliens have abducted two each of six species from Earth. All are currently held captive on a spaceship returning to the alien home-planet, with each species in a different cell. The player's goal is to have one of each type of species escape to the center of the ship. A group of unique individuals, one for each species, will give the player the strength they need to overpower the alien guards and release everyone.

To know more refer https://github.com/sugarlabs/cellgame

## How To Build

```
git clone https://github.com/flathub/org.sugarlabs.CellGame.git
cd org.sugarlabs.CellGame
flatpak -y --user install flathub org.gnome.{Platform,Sdk}//46
flatpak -y --user install org.sugarlabs.BaseApp//24.04
flatpak-builder --user --force-clean --install build org.sugarlabs.CellGame.json
```

## Check For Updates

Install the flatpak external data checker
```
flatpak --user install org.flathub.flatpak-external-data-checker
```

Now to update every single module to the latest stable version use
```
cd org.sugarlabs.CellGame
flatpak run --filesystem=$PWD org.flathub.flatpak-external-data-checker org.sugarlabs.CellGame.json
```
