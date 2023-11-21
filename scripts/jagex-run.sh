#!/bin/sh
winebin="/app/opt/wine/bin"
wineprefix="$XDG_DATA_HOME"/prefix

# Make sure metafile is in the proper location. We do this on each boot to make sure it is replaced after an update.
mkdir -p "$wineprefix/drive_c/users/$(whoami)/AppData/Local/Jagex Launcher"
cp /app/extra/metafile.json "$wineprefix/drive_c/users/$(whoami)/AppData/Local/Jagex Launcher/launcher-win.production.json"

# Make sure the registry has the installation location for runelite.
WINEPREFIX="$wineprefix" WINEDEBUG="-all" "$winebin/wine" reg.exe add "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Uninstall\RuneLite Launcher_is1" /v "InstallLocation" /t REG_SZ /d "Z:\app" /f

# Make sure the registry has the installation location for hdos
WINEPREFIX="$wineprefix" WINEDEBUG="-all" "$winebin/wine" reg.exe add "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Uninstall\HDOS Launcher_is1" /v "InstallLocation" /t REG_SZ /d "Z:\app" /f

# Make sure dxvk is in place in our prefix
cp -r /app/opt/dxvk/x64/*.dll "$wineprefix/drive_c/windows/system32/"
cp -r /app/opt/dxvk/x32/*.dll "$wineprefix/drive_c/windows/syswow64/"

# Run with overrides for dxvk
WINEPREFIX="$wineprefix" WINEDLLOVERRIDES="d3d11=n;d3d10core=n;dxgi=n;d3d9=n" "$winebin/wine" /app/extra/JagexLauncher.exe