import sys

import sqlite3
import gi
gi.require_version("GLib", "2.0")
from gi.repository import GLib

class Connect:

    def conectar(self):
        with sqlite3.connect(database='/app/share/clinicalayudante/clinicalayudante/CAsqlite.db') as conn:
            cursor = conn.cursor()

        return cursor

    
