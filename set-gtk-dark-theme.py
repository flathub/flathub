#!/usr/bin/env python3
"""
Set the GTK titlebar to dark if dark theme is selected in Discord.

Modified from
https://github.com/flathub/com.spotify.Client/blob/master/set-dark-theme-variant.py
"""

import json
import os
import subprocess
import time
import Xlib.display
import Xlib.error
import Xlib.X

disp = Xlib.display.Display()
root = disp.screen().root

NET_CLIENT_LIST = disp.intern_atom("_NET_CLIENT_LIST")
NET_WM_NAME = disp.intern_atom("_NET_WM_NAME")  # UTF-8
WM_NAME = disp.intern_atom("WM_NAME")  # Legacy encoding


# Copied from https://gist.github.com/ssokolow/e7c9aae63fb7973e4d64cff969a78ae8
def _get_window_name_inner(win_obj):
    # Attempts to get the UTF-8 or legacy window name
    for atom in (NET_WM_NAME, WM_NAME):
        try:
            window_name = win_obj.get_full_property(atom, Xlib.X.AnyPropertyType)
        except UnicodeDecodeError:  # Apparently a Debian distro package bug
            return None
        else:
            if window_name:
                win_name = window_name.value
                if isinstance(win_name, bytes):
                    # Apparently COMPOUND_TEXT is so arcane that this is how
                    # tools like xprop deal with receiving it these days
                    win_name = win_name.decode("latin1", "replace")
                return win_name
        return None


def set_theme_variant(window_titles, window_classes, variant):
    # Loop though all of the windows and look for one that matches one
    # of the possible window titles.
    win_ids = root.get_full_property(NET_CLIENT_LIST, Xlib.X.AnyPropertyType)
    if win_ids:
        win_ids = win_ids.value
        for win_id in win_ids:
            try:
                win = disp.create_resource_object("window", win_id)
                # We don't care about dialog windows
                # and the likes.
                if not win.get_wm_transient_for():
                    win_name = _get_window_name_inner(win)
                    win_class = win.get_wm_class()
                    # Only set the theme variant if the following are true:
                    # 1. X window name contains one of the window titles
                    # 2. Window is not the Discord Updater pop-up
                    # 3. X window classes list contains any one of the window classes
                    #    (i.e. the classes match)
                    if (
                        win_name
                        and any([title in win_name for title in window_titles])
                        and win_name != "Discord Updater"
                        and win_class
                        and not set(win_class).isdisjoint(window_classes)
                    ):
                        return set_theme_variant_by_window_id(win_id, variant)
            except Xlib.error.BadWindow:
                pass
    return False


def set_theme_variant_by_window_id(window_id, variant):
    # Pretty self explanatory. Use subprocess to call
    # xprop and set the variant.
    output = subprocess.run(
        [
            "xprop",
            "-id",
            str(window_id),
            "-f",
            "_GTK_THEME_VARIANT",
            "8u",
            "-set",
            "_GTK_THEME_VARIANT",
            variant,
        ]
    )
    return output.returncode == 0


def set_theme_variant_by_window_title(title, variant):
    # Pretty self explanatory. Use subprocess to call
    # xprop and set the variant.
    output = subprocess.run(
        [
            "xprop",
            "-name",
            title,
            "-f",
            "_GTK_THEME_VARIANT",
            "8u",
            "-set",
            "_GTK_THEME_VARIANT",
            variant,
        ]
    )
    return output.returncode == 0


def fallback(window_titles, variant):
    # In the event that the
    # window we were looking for
    # doesn't show up in a timely manner
    # fallback to blindly trying to set
    # the variant.
    for title in window_titles:
        set_theme_variant_by_window_title(title, variant)


def main():
    start = time.time()
    # Listen for X Property Change events.
    root.change_attributes(event_mask=Xlib.X.PropertyChangeMask)
    window_titles = window_classes = ("DiscordPTB", "discord-ptb")
    variant = "dark"
    # In the unlikely event that our window shows up before
    # the script starts don't start a while loop.
    if not set_theme_variant(window_titles, window_classes, variant):
        # Basically give our window 5 secs to show up.
        # If it doesn't just fallback to the old way of blindly
        # trying to set the theme variant
        while True:
            if time.time() - start <= 5:
                # disp.next_event() blocks if no events are
                # queued. In combination with while True
                # it creates a very simple event loop.
                disp.next_event()
                # We only try to set the variant after
                # there has been a Property Change event.
                if set_theme_variant(window_titles, window_classes, variant):
                    break
            else:
                fallback(window_titles, variant)
                break


if __name__ == "__main__":
    xdg_config_home = os.environ.get("XDG_CONFIG_HOME") or os.path.join(
        os.path.expanduser("~"), ".config"
    )
    try:
        with open(
            os.path.join(xdg_config_home, "discordptb", "settings.json"),
            "r",
        ) as settings_file:
            # Read config file and set the dark titlebar if dark theme is enabled
            settings = json.load(settings_file)
            if settings["BACKGROUND_COLOR"] == "#202225":
                main()
    except OSError as e:
        print(f"Error opening settings.json: {e}")
    except json.decoder.JSONDecodeError as e:
        print(f"Error reading settings.json: {e}")
    except KeyError as e:
        print(f"Error reading dictionary property: {e}")
