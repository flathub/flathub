#!/usr/bin/env python3

import sys
import gi

gi.require_version('Gtk', '4.0')
gi.require_version('Adw', '1')
from gi.repository import Gio

from wselector.app import WSelectorApp

def main():
    app = WSelectorApp(
        application_id="io.github.Cookiiieee.WSelector",
        flags=Gio.ApplicationFlags.DEFAULT_FLAGS
    )
    return app.run(sys.argv)

if __name__ == "__main__":
    sys.exit(main())
