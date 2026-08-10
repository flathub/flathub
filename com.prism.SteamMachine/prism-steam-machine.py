#!/usr/bin/env python3
import sys, os, gi
gi.require_version("Gtk", "3.0")
gi.require_version("WebKit2", "4.1")
from gi.repository import Gtk, WebKit2, Gio

APP_URL = os.environ.get("PRISM_APP_URL", "https://prism-news-deck.base44.app")
APP_ID = "com.prism.SteamMachine"

class PrismWindow(Gtk.ApplicationWindow):
    def __init__(self, app):
        super().__init__(application=app, title="PRISM — Steam Machine")
        self.set_default_size(1280, 820)
        self.set_icon_name(APP_ID)
        self._web = WebKit2.WebView()
        self._web.get_settings().set_enable_developer_extras(False)
        self._web.load_uri(APP_URL)
        self.add(self._web)
        self.show_all()

class PrismApp(Gtk.Application):
    def __init__(self):
        super().__init__(application_id=APP_ID, flags=Gio.ApplicationFlags.FLAGS_NONE)
    def do_activate(self):
        if not self.get_active_window():
            PrismWindow(self)
        self.get_active_window().present()

if __name__ == "__main__":
    sys.exit(PrismApp().run(sys.argv))
