# terminal.py - Embeded VTE terminal for gedit
# This file is part of gedit
#
# Copyright (C) 2005-2006 - Paolo Borelli
#
# gedit is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# gedit is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with gedit; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor,
# Boston, MA  02110-1301  USA
#
#
#
#### Edited by: Hamad Al Marri <hamad.s.almarri@gmail.com>
#### Date: Feb 26th, 2020
# 
# 
# 
# 

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('Vte', '2.91')
from gi.repository import GObject, GLib, Gio, Pango, Gdk, Gtk, Vte

from . import commands




class GeditTerminal(Vte.Terminal):

    defaults = {
        'audible_bell'          : False,
    }

    TARGET_URI_LIST = 200

    def __init__(self):
        Vte.Terminal.__init__(self)

        self.set_size(self.get_column_count(), 5)
        self.set_size_request(200, 50)

        tl = Gtk.TargetList.new([])
        tl.add_uri_targets(self.TARGET_URI_LIST)

        self.drag_dest_set(Gtk.DestDefaults.HIGHLIGHT | Gtk.DestDefaults.DROP,
                           [], Gdk.DragAction.DEFAULT | Gdk.DragAction.COPY)
        self.drag_dest_set_target_list(tl)

        self.profile_settings = self.get_profile_settings()
        self.profile_settings.connect("changed", self.on_profile_settings_changed)
        self.system_settings = Gio.Settings.new("org.gnome.desktop.interface")
        self.system_settings.connect("changed::monospace-font-name", self.font_changed)

        self.reconfigure_vte()

        self.spawn_sync(Vte.PtyFlags.DEFAULT, None, [Vte.get_user_shell()], None, GLib.SpawnFlags.SEARCH_PATH, None, None, None)

    def do_drag_data_received(self, drag_context, x, y, data, info, time):
        if info == self.TARGET_URI_LIST:
            self.feed_child(' '.join(["'" + Gio.file_new_for_uri(item).get_path() + "'" for item in Gedit.utils_drop_get_uris(data)]).encode('utf-8'))
            Gtk.drag_finish(drag_context, True, False, time);
        else:
            Vte.Terminal.do_drag_data_received(self, drag_context, x, y, data, info, time)

    def settings_try_new(self, schema):
        schemas = Gio.Settings.list_schemas()
        if not schemas:
            return None

        for s in schemas:
            if s == schema:
                return Gio.Settings.new(schema)

        return None

    def get_profile_settings(self):
        profiles = self.settings_try_new("org.gnome.Terminal.ProfilesList")

        if profiles:
            default_path = "/org/gnome/terminal/legacy/profiles:/:" + profiles.get_string("default") + "/"
            settings = Gio.Settings.new_with_path("org.gnome.Terminal.Legacy.Profile",
                                                  default_path)
        else:
            settings = Gio.Settings.new("org.gnome.gedit.plugins.terminal")

        return settings

    def get_font(self):
        if self.profile_settings.get_boolean("use-system-font"):
            font = self.system_settings.get_string("monospace-font-name")
        else:
            font = self.profile_settings.get_string("font")

        return font

    def font_changed(self, settings=None, key=None):
        font = self.get_font()
        font_desc = Pango.font_description_from_string(font)

        self.set_font(font_desc)

    def reconfigure_vte(self):
        # Fonts
        self.font_changed()

        # colors
        context = self.get_style_context()
        fg = context.get_color(Gtk.StateFlags.NORMAL)
        bg = context.get_background_color(Gtk.StateFlags.NORMAL)
        palette = []

        if not self.profile_settings.get_boolean("use-theme-colors"):
            fg_color = self.profile_settings.get_string("foreground-color")
            if fg_color != "":
                fg = Gdk.RGBA()
                parsed = fg.parse(fg_color)
            bg_color = self.profile_settings.get_string("background-color")
            if bg_color != "":
                bg = Gdk.RGBA()
                parsed = bg.parse(bg_color)
        str_colors = self.profile_settings.get_strv("palette")
        if str_colors:
            for str_color in str_colors:
                try:
                    rgba = Gdk.RGBA()
                    rgba.parse(str_color)
                    palette.append(rgba)
                except:
                    palette = []
                    break

        self.set_colors(fg, bg, palette)
        self.set_cursor_blink_mode(self.profile_settings.get_enum("cursor-blink-mode"))
        self.set_cursor_shape(self.profile_settings.get_enum("cursor-shape"))
        self.set_audible_bell(self.profile_settings.get_boolean("audible-bell"))
        self.set_allow_bold(self.profile_settings.get_boolean("allow-bold"))
        self.set_scroll_on_keystroke(self.profile_settings.get_boolean("scroll-on-keystroke"))
        self.set_scroll_on_output(self.profile_settings.get_boolean("scroll-on-output"))
        self.set_audible_bell(self.defaults['audible_bell'])

        if self.profile_settings.get_boolean("scrollback-unlimited"):
            lines = -1
        else:
            lines = self.profile_settings.get_int("scrollback-lines")
        self.set_scrollback_lines(lines)

    def on_profile_settings_changed(self, settings, key):
        self.reconfigure_vte()
        
        
        
class Plugin():
	
	def __init__(self, app):
		self.name = "terminal"
		self.app = app
		self.signal_handler = app.signal_handler 
		self.commands = []
	

	def activate(self):
		self.signal_handler.key_bindings_to_plugins.append(self)
		commands.set_commands(self)

	
	def key_bindings(self, event, keyval_name, ctrl, alt, shift):
		if ctrl and keyval_name == "t":
			self.show_terminal()
	

	
	def show_terminal(self):
		vte = GeditTerminal() #Vte.Terminal.new()
		w = Gtk.Window.new(Gtk.WindowType.TOPLEVEL)
		
		w.add(vte)
		
		w.show_all() 
		
	
	
	