#!/usr/bin/env python3

import os
import time
import subprocess
import sys

import Xlib
import Xlib.display

disp = Xlib.display.Display()
root = disp.screen().root

NET_CLIENT_LIST = disp.intern_atom('_NET_CLIENT_LIST')

def set_theme_variant(window_titles, variant):
    # Loop though all of the windows and look for one that matches one
    # of the possible window titles.
    win_ids = root.get_full_property(NET_CLIENT_LIST, Xlib.X.AnyPropertyType)
    if win_ids:
        win_ids = win_ids.value
        for win_id in win_ids:
            try:
                win = disp.create_resource_object('window', win_id)
                # We don't care about dialog windows
                # and the likes.
                if not win.get_wm_transient_for():
                    win_name = win.get_wm_name()
                    if win_name and win_name in window_titles:
                        return set_theme_variant_by_window_title(win_name, variant)
            except Xlib.error.BadWindow:
                pass
    return False

def set_theme_variant_by_window_title(title, variant):
    # Pretty self explanatory. Use subprocess to call
    # xprop and set the variant.
    try:
        subprocess.call(['xprop', '-name', title, '-f', '_GTK_THEME_VARIANT', '8u',
            '-set', '_GTK_THEME_VARIANT', variant], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return True
    except:
        return False

def fallback(window_titles, variant):
    # In the event that the
    # window we were looking for
    # doesn't show up in a timely manner
    # fallback to blindly trying to set
    # the variant.
    for title in window_titles:
        set_theme_variant_by_window_title(title, variant)

if __name__ == '__main__':
    if 'SPOTIFYFLATPAK_DISABLE_DARK_TITLEBAR' in os.environ:
        sys.exit(0)

    start = time.time()
    # Listen for X Property Change events.
    root.change_attributes(event_mask=Xlib.X.PropertyChangeMask)
    window_titles = ('Spotify', 'Spotify Free', 'Spotify Premium')
    variant = 'dark'
    # In the unlikely event that our window shows up before
    # the script starts don't start a while loop.
    if not set_theme_variant(window_titles, variant):
        # Basically give our window 2 secs to show up.
        # If it doesn't just fallback to the old way of blindly
        # trying to set the theme variant
        while True:
            if time.time() - start <= 2:
                # disp.next_event() blocks if no events are
                # queued. In combination with while True
                # it creates a very simple event loop.
                disp.next_event()
                # We only try to set the variant after
                # there has been a Property Change event.
                if set_theme_variant(window_titles, variant):
                    break
            else:
                fallback(window_titles, variant)
                break
