#!/usr/bin/env python3
import os
import pwd
import sys
import shutil
import warnings
from urllib.parse import unquote, urlparse
import gi

warnings.filterwarnings("ignore", category=DeprecationWarning)

gi.require_version("Gtk", "4.0")
gi.require_version("Adw", "1")
gi.require_version("Vte", "3.91")

from gi.repository import Gtk, Adw, Vte, GLib, Gio, Pango, Gdk


def parse_rgba(hex_color):
    rgba = Gdk.RGBA()
    rgba.parse(hex_color)
    return rgba


def apply_theme_styles():
    css = """
    window,
    window.background,
    .background,
    headerbar,
    toolbarview {
        background-color: #282c34;
    }
    headerbar {
        background-color: #282c34;
        border: none;
        box-shadow: none;
    }
    .termin-toolbar,
    .termin-headerbar {
        padding: 0;
        margin: 0;
    }
    """
    provider = Gtk.CssProvider()
    if hasattr(provider, "load_from_string"):
        provider.load_from_string(css)
    else:
        provider.load_from_data(css.encode("utf-8"))

    display = Gdk.Display.get_default()
    if display:
        Gtk.StyleContext.add_provider_for_display(
            display,
            provider,
            Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION,
        )


class TerminWindow(Adw.ApplicationWindow):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

        self.current_path = "~"
        self.shell_name = "Shell"
        self.target_scroll_val = None
        self.scroll_tick_id = None
        self.settings = None
        self.portal_proxy = None
        self.is_flatpak = os.path.exists("/.flatpak-info")

        self.set_default_size(800, 500)
        self.set_title(self.shell_name)

        self.toolbar_view = Adw.ToolbarView()
        self.toolbar_view.add_css_class("termin-toolbar")

        self.header_bar = Adw.HeaderBar()
        self.header_bar.add_css_class("termin-headerbar")
        self.toolbar_view.add_top_bar(self.header_bar)

        self.terminal = Vte.Terminal()
        self.terminal.add_css_class("termin-terminal")
        self.terminal.set_vexpand(True)
        self.terminal.set_hexpand(True)
        self.terminal.set_scroll_on_output(False)
        self.terminal.set_scroll_on_keystroke(True)
        self.terminal.set_mouse_autohide(True)
        self.terminal.set_cursor_shape(Vte.CursorShape.IBEAM)

        if hasattr(self.terminal, "set_scroll_unit_is_pixels"):
            self.terminal.set_scroll_unit_is_pixels(True)
        if hasattr(self.terminal, "set_enable_fallback_scrolling"):
            self.terminal.set_enable_fallback_scrolling(False)

        self.setup_colors()

        self.scrolled_window = Gtk.ScrolledWindow()
        self.scrolled_window.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        self.scrolled_window.set_kinetic_scrolling(True)
        self.scrolled_window.set_overlay_scrolling(True)
        self.scrolled_window.set_child(self.terminal)

        self.setup_fonts()
        self.setup_smooth_scrolling()
        self.terminal.connect("window-title-changed", self.on_window_title_changed)
        self.terminal.connect("current-directory-uri-changed", self.on_directory_changed)
        self.terminal.connect("child-exited", self.on_child_exited)
        self.terminal.connect("realize", self.on_terminal_realize)

        self.setup_key_controllers()

        self.toolbar_view.set_content(self.scrolled_window)
        self.set_content(self.toolbar_view)

        self.spawn_session()

    def on_terminal_realize(self, widget):
        self.update_terminal_margins()

    def update_terminal_margins(self):
        char_width = self.terminal.get_char_width()
        if char_width > 0:
            self.terminal.set_margin_start(char_width)
            self.terminal.set_margin_end(char_width)

    def on_window_title_changed(self, terminal):
        title = terminal.get_window_title() if hasattr(terminal, "get_window_title") else None
        if title:
            self.set_title(title)
        else:
            self.set_title(self.shell_name)

    def setup_colors(self):
        bg = parse_rgba("#282c34")
        fg = parse_rgba("#abb2bf")
        cursor = parse_rgba("#ffffff")

        palette_hex = [
            "#282c34",
            "#e06c75",
            "#98c379",
            "#e5c07b",
            "#61afef",
            "#c678dd",
            "#56b6c2",
            "#abb2bf",
            "#5c6370",
            "#be5046",
            "#98c379",
            "#e5c07b",
            "#61afef",
            "#c678dd",
            "#56b6c2",
            "#ffffff",
        ]
        palette = [parse_rgba(c) for c in palette_hex]

        self.terminal.set_colors(fg, bg, palette)
        if hasattr(self.terminal, "set_color_cursor"):
            self.terminal.set_color_cursor(cursor)
        if hasattr(self.terminal, "set_color_cursor_foreground"):
            self.terminal.set_color_cursor_foreground(bg)

    def setup_smooth_scrolling(self):
        controller = Gtk.EventControllerScroll.new(
            Gtk.EventControllerScrollFlags.BOTH_AXES
            | Gtk.EventControllerScrollFlags.KINETIC
        )
        controller.set_propagation_phase(Gtk.PropagationPhase.CAPTURE)
        controller.connect("scroll", self.on_scroll)
        self.scrolled_window.add_controller(controller)

    def on_scroll(self, controller, dx, dy):
        adj = self.scrolled_window.get_vadjustment()
        if not adj:
            return False

        step = adj.get_step_increment()
        if step <= 0:
            step = 30.0

        if self.target_scroll_val is None:
            self.target_scroll_val = adj.get_value()

        lower = adj.get_lower()
        upper = adj.get_upper() - adj.get_page_size()
        if upper < lower:
            upper = lower

        scroll_distance = dy * (step * 2.5 if abs(dy) <= 1.0 else dy * 4.0)
        self.target_scroll_val = max(lower, min(upper, self.target_scroll_val + scroll_distance))

        if self.scroll_tick_id is None:
            self.scroll_tick_id = self.scrolled_window.add_tick_callback(self.on_scroll_tick)

        return True

    def on_scroll_tick(self, widget, frame_clock):
        adj = self.scrolled_window.get_vadjustment()
        if not adj or self.target_scroll_val is None:
            self.scroll_tick_id = None
            return GLib.SOURCE_REMOVE

        curr = adj.get_value()
        diff = self.target_scroll_val - curr

        if abs(diff) < 0.5:
            adj.set_value(self.target_scroll_val)
            self.target_scroll_val = None
            self.scroll_tick_id = None
            return GLib.SOURCE_REMOVE

        adj.set_value(curr + diff * 0.22)
        return GLib.SOURCE_CONTINUE

    def setup_key_controllers(self):
        controller = Gtk.EventControllerKey.new()
        controller.set_propagation_phase(Gtk.PropagationPhase.CAPTURE)
        controller.connect("key-pressed", self.on_key_pressed)
        self.add_controller(controller)

    def on_key_pressed(self, controller, keyval, keycode, state):
        modifiers = state & (Gdk.ModifierType.CONTROL_MASK | Gdk.ModifierType.SHIFT_MASK)
        ctrl_shift = Gdk.ModifierType.CONTROL_MASK | Gdk.ModifierType.SHIFT_MASK

        if modifiers == ctrl_shift:
            if keyval in (Gdk.KEY_c, Gdk.KEY_C):
                self.terminal.copy_clipboard_format(Vte.Format.TEXT)
                return True
            if keyval in (Gdk.KEY_v, Gdk.KEY_V):
                self.terminal.paste_clipboard()
                return True
            if keyval in (Gdk.KEY_a, Gdk.KEY_A):
                self.terminal.select_all()
                return True

        return False

    def on_directory_changed(self, terminal):
        uri = terminal.get_current_directory_uri()
        if not uri:
            return

        parsed = urlparse(uri)
        path = unquote(parsed.path)
        home = GLib.get_home_dir()

        if path == home:
            self.current_path = "~"
        elif path.startswith(home + "/"):
            self.current_path = "~" + path[len(home):]
        else:
            self.current_path = path

    def setup_fonts(self):
        if self.is_flatpak:
            try:
                self.portal_proxy = Gio.DBusProxy.new_for_bus_sync(
                    Gio.BusType.SESSION,
                    Gio.DBusProxyFlags.NONE,
                    None,
                    "org.freedesktop.portal.Desktop",
                    "/org/freedesktop/portal/desktop",
                    "org.freedesktop.portal.Settings",
                    None,
                )
                self.portal_proxy.connect("g-signal", self.on_portal_setting_changed)
            except Exception:
                self.portal_proxy = None
        else:
            schema_id = "org.gnome.desktop.interface"
            source = Gio.SettingsSchemaSource.get_default()
            if source and source.lookup(schema_id, True):
                try:
                    self.settings = Gio.Settings.new(schema_id)
                    self.settings.connect("changed::monospace-font-name", self.on_font_changed)
                except Exception:
                    self.settings = None

        self.apply_font()

    def on_portal_setting_changed(self, proxy, sender_name, signal_name, parameters):
        if signal_name == "SettingChanged":
            try:
                namespace, key, value = parameters.unpack()
                if namespace == "org.gnome.desktop.interface" and key == "monospace-font-name":
                    val = value
                    while isinstance(val, GLib.Variant):
                        val = val.unpack()
                    if isinstance(val, str) and val.strip():
                        self.set_terminal_font(val.strip())
            except Exception:
                pass

    def on_font_changed(self, settings, key):
        self.apply_font()

    def read_portal_font(self):
        if not self.portal_proxy:
            return None
        try:
            res = self.portal_proxy.call_sync(
                "Read",
                GLib.Variant("(ss)", ("org.gnome.desktop.interface", "monospace-font-name")),
                Gio.DBusCallFlags.NONE,
                1000,
                None,
            )
            if res:
                outer = res.unpack()
                if outer and len(outer) > 0:
                    val = outer[0]
                    while isinstance(val, GLib.Variant):
                        val = val.unpack()
                    if isinstance(val, str) and val.strip():
                        return val.strip()
        except Exception:
            pass
        return None

    def read_host_gsettings_font(self):
        if not self.is_flatpak:
            return None
        try:
            res = GLib.spawn_command_line_sync(
                "flatpak-spawn --host gsettings get org.gnome.desktop.interface monospace-font-name"
            )
            if res and res[0] and res[1]:
                val = res[1].decode("utf-8", errors="ignore").strip().strip("'").strip('"')
                if val:
                    return val
        except Exception:
            pass
        return None

    def get_system_monospace_font(self):
        if self.is_flatpak:
            portal_font = self.read_portal_font()
            if portal_font:
                return portal_font

            host_font = self.read_host_gsettings_font()
            if host_font:
                return host_font

        if self.settings:
            try:
                font = self.settings.get_string("monospace-font-name")
                if font:
                    return font
            except Exception:
                pass

        return "Monospace 11"

    def apply_font(self):
        font_name = self.get_system_monospace_font()
        self.set_terminal_font(font_name)

    def set_terminal_font(self, font_name):
        if not font_name:
            font_name = "Monospace 11"
        desc = Pango.FontDescription.from_string(font_name)
        self.terminal.set_font(desc)
        self.update_terminal_margins()

    def detect_shell(self):
        if self.is_flatpak:
            try:
                user = GLib.get_user_name()
                res = GLib.spawn_command_line_sync(f"flatpak-spawn --host getent passwd {user}")
                if res and res[0] and res[1]:
                    entry = res[1].decode("utf-8", errors="ignore").strip().split(":")
                    if len(entry) >= 7 and entry[6] and os.path.isabs(entry[6]):
                        return entry[6]
            except Exception:
                pass

            try:
                res = GLib.spawn_command_line_sync("flatpak-spawn --host printenv SHELL")
                if res and res[0] and res[1]:
                    shell = res[1].decode("utf-8", errors="ignore").strip()
                    if shell and os.path.isabs(shell):
                        return shell
            except Exception:
                pass

        try:
            return pwd.getpwuid(os.getuid()).pw_shell
        except Exception:
            return os.environ.get("SHELL", "/bin/bash")

    def spawn_session(self):
        shell_path = self.detect_shell()
        raw_name = os.path.basename(shell_path)
        self.shell_name = raw_name.capitalize() if raw_name else "Shell"
        self.set_title(self.shell_name)

        working_dir = GLib.get_home_dir()

        if self.is_flatpak:
            pty_flags = Vte.PtyFlags.DEFAULT
            host_spawn_bin = "/app/bin/host-spawn" if os.path.exists("/app/bin/host-spawn") else "host-spawn"
            argv = [
                host_spawn_bin,
                shell_path,
            ]
            envv = GLib.get_environ()
        else:
            pty_flags = Vte.PtyFlags.DEFAULT
            argv = [shell_path]
            envv = GLib.environ_setenv(GLib.get_environ(), "TERM_PROGRAM", "Termin", True)
            envv = GLib.environ_setenv(envv, "COLORTERM", "truecolor", True)
            envv = GLib.environ_setenv(envv, "TERM", "xterm-256color", True)

        self.terminal.spawn_async(
            pty_flags=pty_flags,
            working_directory=working_dir,
            argv=argv,
            envv=envv,
            spawn_flags=GLib.SpawnFlags.SEARCH_PATH,
            timeout=-1,
        )

    def on_child_exited(self, terminal, status):
        self.close()


class TerminApp(Adw.Application):
    def __init__(self):
        super().__init__(
            application_id="io.github.theoninesixy.Termin",
            flags=Gio.ApplicationFlags.FLAGS_NONE,
        )

    def do_activate(self):
        manager = Adw.StyleManager.get_default()
        manager.set_color_scheme(Adw.ColorScheme.FORCE_DARK)
        apply_theme_styles()

        win = self.props.active_window
        if not win:
            win = TerminWindow(application=self)
        win.present()


def main():
    app = TerminApp()
    return app.run(sys.argv)


if __name__ == "__main__":
    sys.exit(main())