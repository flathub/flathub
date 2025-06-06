#!/bin/bash

# Script to set wallpaper across different desktop environments
# Usage: ./set_wallpaper.sh /path/to/image.jpg

# Check if file path is provided
if [ -z "$1" ]; then
    echo "Error: No wallpaper path provided"
    echo "Usage: $0 /path/to/image.jpg"
    exit 1
fi

# Check if the file exists
if [ ! -f "$1" ]; then
    echo "Error: File does not exist: $1"
    exit 1
fi

# Get absolute path
WALLPAPER=$(realpath "$1")
echo "Setting wallpaper: $WALLPAPER"

# Function to set wallpaper in GNOME
set_gnome_wallpaper() {
    echo "Detected GNOME desktop environment"
    
    # Convert to URI
    URI="file://$WALLPAPER"
    
    # Try gsettings command
    gsettings set org.gnome.desktop.background picture-uri "$URI"
    gsettings set org.gnome.desktop.background picture-uri-dark "$URI"
    
    # Set picture options (fill, centered, scaled, etc.)
    gsettings set org.gnome.desktop.background picture-options 'zoom'
    
    echo "Wallpaper set using gsettings"
}

# Function to set wallpaper in KDE Plasma
set_kde_wallpaper() {
    echo "Detected KDE Plasma desktop environment"
    
    # Use KDE's scripting interface
    qdbus org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript "
        var allDesktops = desktops();
        for (i=0; i<allDesktops.length; i++) {
            d = allDesktops[i];
            d.wallpaperPlugin = 'org.kde.image';
            d.currentConfigGroup = Array('Wallpaper', 'org.kde.image', 'General');
            d.writeConfig('Image', '$WALLPAPER');
        }
    "
    
    echo "Wallpaper set using KDE scripting"
}

# Function to set wallpaper in XFCE
set_xfce_wallpaper() {
    echo "Detected XFCE desktop environment"
    
    # Set wallpaper for all monitors
    for screen in $(xfconf-query -c xfce4-desktop -l | grep last-image); do
        xfconf-query -c xfce4-desktop -p "$screen" -s "$WALLPAPER"
    done
    
    echo "Wallpaper set using xfconf-query"
}

# Function to set wallpaper using feh (for i3, openbox, etc.)
set_feh_wallpaper() {
    echo "Using feh to set wallpaper"
    
    # Set wallpaper using feh
    feh --bg-fill "$WALLPAPER"
    
    echo "Wallpaper set using feh"
}

# Function to set wallpaper using nitrogen
set_nitrogen_wallpaper() {
    echo "Using nitrogen to set wallpaper"
    
    # Set wallpaper using nitrogen
    nitrogen --set-zoom-fill "$WALLPAPER"
    
    echo "Wallpaper set using nitrogen"
}

# Detect desktop environment and set wallpaper accordingly
if [ "$XDG_CURRENT_DESKTOP" = "GNOME" ] || [ "$XDG_CURRENT_DESKTOP" = "ubuntu:GNOME" ]; then
    set_gnome_wallpaper
elif [ "$XDG_CURRENT_DESKTOP" = "KDE" ] || [ "$XDG_CURRENT_DESKTOP" = "Plasma" ]; then
    set_kde_wallpaper
elif [ "$XDG_CURRENT_DESKTOP" = "XFCE" ]; then
    set_xfce_wallpaper
elif command -v feh >/dev/null 2>&1; then
    set_feh_wallpaper
elif command -v nitrogen >/dev/null 2>&1; then
    set_nitrogen_wallpaper
else
    # Fallback to GNOME method
    echo "Desktop environment not detected, trying GNOME method"
    set_gnome_wallpaper
fi

echo "Wallpaper setting completed"
exit 0
