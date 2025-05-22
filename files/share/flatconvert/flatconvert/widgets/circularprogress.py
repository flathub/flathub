# circularprogress.py
#
# Copyright 2024-2025 Quentin Soranzo Krebs
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: GPL-3.0-or-later

from flatconvert.utils import *
from gi.repository import Adw
from gi.repository import Gtk, Gdk, cairo, Graphene, GObject


class CircularProgress(Gtk.Widget):
    """
    A custom GTK widget that displays a circular progress indicator.
    Attributes:
        fraction (float): The progress value between 0.0 and 1.0.
        accent_color (Gdk.RGBA): The accent color used for the progress arc.
    Signals:
        clicked: Emitted when the widget is clicked with the primary mouse button.
    Methods:
        __init__():
            Initializes the CircularProgress widget, setting default values and
            retrieving the accent color from the style manager.
        do_snapshot(snapshot):
            Renders the widget using the provided snapshot. Draws a circular
            background and an arc representing the progress.
        set_fraction(fraction):
            Sets the progress value and schedules a redraw of the widget.
        on_button_press(widget, event):
            Handles button press events. Emits the 'clicked' signal if the
            primary mouse button is pressed.
    """


    __gtype_name__ = "CircularProgress"

    __gsignals__ = {
        'clicked': (GObject.SIGNAL_RUN_FIRST, None, ())
    }

    def __init__(self):
        """
        Initializes the CircularProgress widget, setting default values and
        retrieving the accent color from the style manager.
        """
        super().__init__()
        self.fraction = 0.0  # Progression entre 0.0 et 1.0
        self.set_hexpand(True)
        self.set_vexpand(True)
        style_manager = Adw.StyleManager.get_default()
        self.accent_color = style_manager.get_accent_color_rgba()

    def do_snapshot(self, snapshot):
        """
        Renders the widget using the provided snapshot. Draws a circular
        background and an arc representing the progress.
        Args:
            snapshot (Gtk.Snapshot): The snapshot to render the widget.
        """
        
        # Get widget allocated size
        alloc = self.get_allocation()
        width, height = alloc.width, alloc.height
        diameter = min(width, height)
        radius = diameter / 2.0 - 5  # Ajouter une marge de 5 pixels
        cx = width / 2.0
        cy = height / 2.0

        # Get the accent color
        color = self.accent_color

        # Create a rectangle to contain the drawing
        rect = Graphene.Rect()
        rect.init(0, 0, width, height)

        # Obtenir un contexte Cairo à partir du snapshot
        cr = snapshot.append_cairo(rect)

        # Dessiner le cercle de fond (en gris)
        cr.set_line_width(10)
        cr.move_to(cx, cy)
        cr.arc(cx, cy, radius, 0, 2 * 3.14159)
        cr.close_path()
        cr.set_source_rgba(0.8, 0.8, 0.8, 1)  # Gris clair
        cr.fill()
        cr.stroke()

        # Draw the progress arc
        start_angle = -3.14159 / 2  # Commencer en haut
        end_angle = start_angle + self.fraction * 2 * 3.14159
        cr.move_to(cx, cy)
        cr.arc(cx, cy, radius, start_angle, end_angle)
        cr.close_path()
        cr.set_source_rgba(color.red, color.green,
                           color.blue, color.alpha)  # Bleu
        cr.fill()
        cr.stroke()

    def set_fraction(self, fraction : float):
        """
        Sets the progress value and schedules a redraw of the widget.
        Args:
            fraction (float): The progress value between 0.0 and 1.0.
        """
        self.fraction = fraction
        self.queue_draw()

    def on_button_press(self, widget, event):
        """
        Handles button press events. Emits the 'clicked' signal if the
        primary mouse button is pressed.
        """
        if event.button == Gdk.BUTTON_PRIMARY:
            self.emit('clicked')


GObject.type_register(CircularProgress)
