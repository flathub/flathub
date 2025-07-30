#!/usr/bin/env python3
import gi, math
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

BUTTONS = [
    ["7", "8", "9", "/"],
    ["4", "5", "6", "*"],
    ["1", "2", "3", "-"],
    ["0", ".", "=", "+"],
]

class Calc(Gtk.Window):
    def __init__(self):
        super().__init__(title="SimpleCalc")
        self.set_border_width(10)
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        self.add(vbox)

        self.entry = Gtk.Entry()
        self.entry.set_alignment(1)           # текст справа
        self.entry.set_editable(False)
        vbox.pack_start(self.entry, False, False, 0)

        grid = Gtk.Grid(column_spacing=6, row_spacing=6)
        vbox.pack_start(grid, True, True, 0)

        for r, row in enumerate(BUTTONS):
            for c, label in enumerate(row):
                btn = Gtk.Button(label=label)
                btn.connect("clicked", self.on_click)
                grid.attach(btn, c, r, 1, 1)

    def on_click(self, button):
        label = button.get_label()
        if label == "=":
            try:
                res = str(eval(self.entry.get_text(), {"__builtins__": {}}, vars(math)))
            except Exception:
                res = "Err"
            self.entry.set_text(res)
        else:
            self.entry.set_text(self.entry.get_text() + label)

if __name__ == "__main__":
    win = Calc()
    win.connect("destroy", Gtk.main_quit)
    win.show_all()
    Gtk.main()
