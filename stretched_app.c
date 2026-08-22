#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    // Force monitor hardware to stretch completely full-screen via X11 xrandr
    system("flatpak-spawn --host xrandr --output eDP --set \"scaling mode\" \"Full\"");
    system("flatpak-spawn --host xrandr --output eDP --mode 1024x768");

    // Open the packaged standalone x86_64 file directly via desktop runtime player
    system("xdg-open /app/share/roblox/roblox_x86_64.apk");

    // Keep launcher active to monitor runtime state before resetting dimensions
    sleep(5);

    // Revert display scaling layout safely back to normal upon app exit
    system("flatpak-spawn --host xrandr --output eDP --set \"scaling mode\" \"None\"");
    system("flatpak-spawn --host xrandr --output eDP --mode 1024x768");

    return 0;
}
