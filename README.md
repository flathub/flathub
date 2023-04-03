To copy configuration:
```
mkdir -p ~/.supermodel/Analysis/
mkdir -p ~/.supermodel/Assets/
mkdir -p ~/.supermodel/Config/
mkdir -p ~/.supermodel/Log/
mkdir -p ~/.supermodel/NVRAM/
mkdir -p ~/.supermodel/Saves/
mkdir -p ~/.supermodel/Screenshots/
cp -r ~/.local/share/flatpak/app/com.supermodel3.Supermodel/current/active/files/Assets ~/.supermodel/
cp ~/.local/share/flatpak/app/com.supermodel3.Supermodel/current/active/files/Config/Games.xml ~/.supermodel/Config/
cp -n ~/.local/share/flatpak/app/com.supermodel3.Supermodel/current/active/files/Config/Supermodel.ini ~/.supermodel/Config/
```
To run from a terminal:
```
flatpak run com.supermodel3.Supermodel <romset>
```
