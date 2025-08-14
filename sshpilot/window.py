"""
Main Window for sshPilot
Primary UI with connection list, tabs, and terminal management
"""

import os
import logging
from typing import Optional, Dict, Any, List, Tuple

import gi
gi.require_version('Gtk', '4.0')
gi.require_version('Adw', '1')
try:
    gi.require_version('Vte', '3.91')
    from gi.repository import Vte
    _HAS_VTE = True
except Exception:
    _HAS_VTE = False

from gi.repository import Gtk, Adw, Gio, GLib, GObject, Gdk, Pango

# Feature detection for libadwaita versions across distros
HAS_OVERLAY_SPLIT = hasattr(Adw, 'OverlaySplitView')

from gettext import gettext as _

from .connection_manager import ConnectionManager, Connection
from .terminal import TerminalWidget
from .config import Config
from .key_manager import KeyManager, SSHKey
from .port_forwarding_ui import PortForwardingRules
from .connection_dialog import ConnectionDialog

logger = logging.getLogger(__name__)

class ConnectionRow(Gtk.ListBoxRow):
    """Row widget for connection list"""
    
    def __init__(self, connection: Connection):
        super().__init__()
        self.connection = connection
        
        # Create main box
        box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        box.set_margin_start(12)
        box.set_margin_end(12)
        box.set_margin_top(6)
        box.set_margin_bottom(6)
        
        # Connection icon
        icon = Gtk.Image.new_from_icon_name('computer-symbolic')
        icon.set_icon_size(Gtk.IconSize.NORMAL)
        box.append(icon)
        
        # Connection info
        info_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=2)
        info_box.set_hexpand(True)
        
        # Nickname label
        self.nickname_label = Gtk.Label()
        self.nickname_label.set_markup(f"<b>{connection.nickname}</b>")
        self.nickname_label.set_halign(Gtk.Align.START)
        info_box.append(self.nickname_label)
        
        # Host info label (may be hidden based on user setting)
        self.host_label = Gtk.Label()
        self.host_label.set_halign(Gtk.Align.START)
        self.host_label.add_css_class('dim-label')
        self._apply_host_label_text()
        info_box.append(self.host_label)
        
        box.append(info_box)
        
        # Port forwarding indicators (L/R/D)
        self.indicator_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        self.indicator_box.set_halign(Gtk.Align.CENTER)
        self.indicator_box.set_valign(Gtk.Align.CENTER)
        box.append(self.indicator_box)

        # Connection status indicator
        self.status_icon = Gtk.Image.new_from_icon_name('network-offline-symbolic')
        self.status_icon.set_pixel_size(16)  # GTK4 uses pixel size instead of IconSize
        box.append(self.status_icon)
        
        self.set_child(box)
        self.set_selectable(True)  # Make the row selectable for keyboard navigation
        
        # Update status
        self.update_status()
        # Update forwarding indicators
        self._update_forwarding_indicators()

    @staticmethod
    def _install_pf_css():
        try:
            # Install CSS for port forwarding indicator badges once per display
            display = Gdk.Display.get_default()
            if not display:
                return
            # Use an attribute on the display to avoid re-adding provider
            if getattr(display, '_pf_css_installed', False):
                return
            provider = Gtk.CssProvider()
            css = """
            .pf-indicator { /* kept for legacy, not used by circled glyphs */ }
            .pf-local { color: #E01B24; }
            .pf-remote { color: #2EC27E; }
            .pf-dynamic { color: #3584E4; }
            """
            provider.load_from_data(css.encode('utf-8'))
            Gtk.StyleContext.add_provider_for_display(display, provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
            setattr(display, '_pf_css_installed', True)
        except Exception:
            pass

    def _update_forwarding_indicators(self):
        # Ensure CSS exists
        self._install_pf_css()
        # Clear previous indicators
        try:
            while self.indicator_box.get_first_child():
                self.indicator_box.remove(self.indicator_box.get_first_child())
        except Exception:
            return

        rules = getattr(self.connection, 'forwarding_rules', []) or []
        has_local = any(r.get('enabled', True) and r.get('type') == 'local' for r in rules)
        has_remote = any(r.get('enabled', True) and r.get('type') == 'remote' for r in rules)
        has_dynamic = any(r.get('enabled', True) and r.get('type') == 'dynamic' for r in rules)

        def make_badge(letter: str, cls: str):
            # Use Unicode precomposed circled letters for perfect centering
            circled_map = {
                'L': '\u24C1',  # Ⓛ
                'R': '\u24C7',  # Ⓡ
                'D': '\u24B9',  # Ⓓ
            }
            glyph = circled_map.get(letter, letter)
            lbl = Gtk.Label(label=glyph)
            lbl.add_css_class(cls)
            lbl.set_halign(Gtk.Align.CENTER)
            lbl.set_valign(Gtk.Align.CENTER)
            try:
                lbl.set_xalign(0.5)
                lbl.set_yalign(0.5)
            except Exception:
                pass
            return lbl

        if has_local:
            self.indicator_box.append(make_badge('L', 'pf-local'))
        if has_remote:
            self.indicator_box.append(make_badge('R', 'pf-remote'))
        if has_dynamic:
            self.indicator_box.append(make_badge('D', 'pf-dynamic'))

    def _apply_host_label_text(self):
        try:
            window = self.get_root()
            hide = bool(getattr(window, '_hide_hosts', False)) if window else False
        except Exception:
            hide = False
        if hide:
            self.host_label.set_text('••••••••••')
        else:
            self.host_label.set_text(f"{self.connection.username}@{self.connection.host}")

    def apply_hide_hosts(self, hide: bool):
        """Called by window when hide/show toggles."""
        self._apply_host_label_text()

    def update_status(self):
        """Update connection status display"""
        try:
            # Check if there's any active terminal for this connection
            window = self.get_root()
            has_active_terminal = False

            # Prefer multi-tab map if present; fallback to most-recent mapping
            if hasattr(window, 'connection_to_terminals') and self.connection in getattr(window, 'connection_to_terminals', {}):
                for t in window.connection_to_terminals.get(self.connection, []) or []:
                    if getattr(t, 'is_connected', False):
                        has_active_terminal = True
                        break
            elif hasattr(window, 'active_terminals') and self.connection in window.active_terminals:
                terminal = window.active_terminals[self.connection]
                # Check if the terminal is still valid and connected
                if terminal and hasattr(terminal, 'is_connected'):
                    has_active_terminal = terminal.is_connected
            
            # Update the connection's is_connected status
            self.connection.is_connected = has_active_terminal
            
            # Log the status update for debugging
            logger.debug(f"Updating status for {self.connection.nickname}: is_connected={has_active_terminal}")
            
            # Update the UI based on the connection status
            if has_active_terminal:
                self.status_icon.set_from_icon_name('network-idle-symbolic')
                self.status_icon.set_tooltip_text(f'Connected to {getattr(self.connection, "hname", "") or self.connection.host}')
                logger.debug(f"Set status icon to connected for {self.connection.nickname}")
            else:
                self.status_icon.set_from_icon_name('network-offline-symbolic')
                self.status_icon.set_tooltip_text('Disconnected')
                logger.debug(f"Set status icon to disconnected for {getattr(self.connection, 'nickname', 'connection')}")
                
            # Force a redraw to ensure the icon updates
            self.status_icon.queue_draw()
            
        except Exception as e:
            logger.error(f"Error updating status for {getattr(self.connection, 'nickname', 'connection')}: {e}")
    
    def update_display(self):
        """Update the display with current connection data"""
        # Update the labels with current connection data
        if hasattr(self.connection, 'nickname') and hasattr(self, 'nickname_label'):
            self.nickname_label.set_markup(f"<b>{self.connection.nickname}</b>")
        
        if hasattr(self.connection, 'username') and hasattr(self.connection, 'host') and hasattr(self, 'host_label'):
            port_text = f":{self.connection.port}" if hasattr(self.connection, 'port') and self.connection.port != 22 else ""
            self.host_label.set_text(f"{self.connection.username}@{self.connection.host}{port_text}")
        # Refresh forwarding indicators if rules changed
        self._update_forwarding_indicators()
        
        self.update_status()

    def show_error(self, message):
        """Show error message"""
        dialog = Adw.MessageDialog(
            transient_for=self,
            heading='Error',
            body=message,
        )
        dialog.add_response('ok', 'OK')
        dialog.set_default_response('ok')
        dialog.present()

class WelcomePage(Gtk.Box):
    """Welcome page shown when no tabs are open"""
    
    def __init__(self):
        super().__init__(orientation=Gtk.Orientation.VERTICAL, spacing=24)
        self.set_valign(Gtk.Align.CENTER)
        self.set_halign(Gtk.Align.CENTER)
        self.set_margin_start(48)
        self.set_margin_end(48)
        self.set_margin_top(48)
        self.set_margin_bottom(48)
        
        # Welcome icon
        try:
            texture = Gdk.Texture.new_from_resource('/io/github/mfat/sshpilot/sshpilot.svg')
            icon = Gtk.Image.new_from_paintable(texture)
            icon.set_pixel_size(128)
        except Exception:
            icon = Gtk.Image.new_from_icon_name('network-workgroup-symbolic')
            icon.set_icon_size(Gtk.IconSize.LARGE)
            icon.set_pixel_size(128)
        self.append(icon)
        
        # Welcome message
        message = Gtk.Label()
        message.set_text('Select a host from the list, double-click or press Enter to connect')
        message.set_halign(Gtk.Align.CENTER)
        message.add_css_class('dim-label')
        self.append(message)
        
        # Shortcuts box
        shortcuts_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        shortcuts_box.set_halign(Gtk.Align.CENTER)
        
        shortcuts_title = Gtk.Label()
        shortcuts_title.set_markup('<b>Keyboard Shortcuts</b>')
        shortcuts_box.append(shortcuts_title)
        
        shortcuts = [
            ('Ctrl+N', 'New Connection'),
            ('Ctrl+L', 'Focus connection list to select server'),
            ('Ctrl+Shift+K', 'New SSH Key'),
            ('Alt+Right', 'Next Tab'),
            ('Alt+Left', 'Previous Tab'),
            ('Ctrl+F4', 'Close Tab'),
            ('Ctrl+,', 'Preferences'),
        ]
        
        for shortcut, description in shortcuts:
            shortcut_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
            
            key_label = Gtk.Label()
            key_label.set_markup(f'<tt>{shortcut}</tt>')
            key_label.set_width_chars(15)
            key_label.set_halign(Gtk.Align.START)
            shortcut_box.append(key_label)
            
            desc_label = Gtk.Label()
            desc_label.set_text(description)
            desc_label.set_halign(Gtk.Align.START)
            shortcut_box.append(desc_label)
            
            shortcuts_box.append(shortcut_box)
        
        self.append(shortcuts_box)

class PreferencesWindow(Adw.PreferencesWindow):
    """Preferences dialog window"""
    
    def __init__(self, parent_window, config):
        super().__init__()
        self.set_transient_for(parent_window)
        self.set_modal(True)
        self.config = config
        
        # Set window properties
        self.set_title("Preferences")
        self.set_default_size(600, 500)
        
        # Initialize the preferences UI
        self.setup_preferences()

        # Save on close to persist advanced SSH settings
        self.connect('close-request', self.on_close_request)
    
    def setup_preferences(self):
        """Set up preferences UI with current values"""
        try:
            # Create Terminal preferences page
            terminal_page = Adw.PreferencesPage()
            terminal_page.set_title("Terminal")
            terminal_page.set_icon_name("utilities-terminal-symbolic")
            
            # Terminal appearance group
            appearance_group = Adw.PreferencesGroup()
            appearance_group.set_title("Appearance")
            
            # Font selection row
            self.font_row = Adw.ActionRow()
            self.font_row.set_title("Font")
            current_font = self.config.get_setting('terminal-font', 'Monospace 12')
            self.font_row.set_subtitle(current_font)
            
            font_button = Gtk.Button()
            font_button.set_label("Choose")
            font_button.connect('clicked', self.on_font_button_clicked)
            self.font_row.add_suffix(font_button)
            
            appearance_group.add(self.font_row)
            
            # Terminal color scheme
            self.color_scheme_row = Adw.ComboRow()
            self.color_scheme_row.set_title("Color Scheme")
            self.color_scheme_row.set_subtitle("Terminal color theme")
            
            color_schemes = Gtk.StringList()
            color_schemes.append("Default")
            color_schemes.append("Solarized Dark")
            color_schemes.append("Solarized Light")
            color_schemes.append("Monokai")
            color_schemes.append("Dracula")
            color_schemes.append("Nord")
            color_schemes.append("Gruvbox Dark")
            color_schemes.append("One Dark")
            color_schemes.append("Tomorrow Night")
            color_schemes.append("Material Dark")
            self.color_scheme_row.set_model(color_schemes)
            
            # Set current color scheme from config
            current_scheme_key = self.config.get_setting('terminal.theme', 'default')
            
            # Get the display name for the current scheme key
            theme_mapping = self.get_theme_name_mapping()
            reverse_mapping = {v: k for k, v in theme_mapping.items()}
            current_scheme_display = reverse_mapping.get(current_scheme_key, 'Default')
            
            # Find the index of the current scheme in the dropdown
            scheme_names = [
                "Default", "Solarized Dark", "Solarized Light",
                "Monokai", "Dracula", "Nord",
                "Gruvbox Dark", "One Dark", "Tomorrow Night", "Material Dark"
            ]
            try:
                current_index = scheme_names.index(current_scheme_display)
                self.color_scheme_row.set_selected(current_index)
            except ValueError:
                # If the saved scheme isn't found, default to the first option
                self.color_scheme_row.set_selected(0)
                # Also update the config to use the default value
                self.config.set_setting('terminal.theme', 'default')
            
            self.color_scheme_row.connect('notify::selected', self.on_color_scheme_changed)
            
            appearance_group.add(self.color_scheme_row)
            terminal_page.add(appearance_group)
            
            # Create Interface preferences page
            interface_page = Adw.PreferencesPage()
            interface_page.set_title("Interface")
            interface_page.set_icon_name("applications-graphics-symbolic")
            
            # Behavior group
            behavior_group = Adw.PreferencesGroup()
            behavior_group.set_title("Behavior")
            
            # Confirm before disconnecting
            self.confirm_disconnect_switch = Adw.SwitchRow()
            self.confirm_disconnect_switch.set_title("Confirm before disconnecting")
            self.confirm_disconnect_switch.set_subtitle("Show a confirmation dialog when disconnecting from a host")
            self.confirm_disconnect_switch.set_active(
                self.config.get_setting('confirm-disconnect', True)
            )
            self.confirm_disconnect_switch.connect('notify::active', self.on_confirm_disconnect_changed)
            behavior_group.add(self.confirm_disconnect_switch)
            
            interface_page.add(behavior_group)
            
            # Appearance group
            interface_appearance_group = Adw.PreferencesGroup()
            interface_appearance_group.set_title("Appearance")
            
            # Theme selection
            self.theme_row = Adw.ComboRow()
            self.theme_row.set_title("Application Theme")
            self.theme_row.set_subtitle("Choose light, dark, or follow system theme")
            
            themes = Gtk.StringList()
            themes.append("Follow System")
            themes.append("Light")
            themes.append("Dark")
            self.theme_row.set_model(themes)
            
            # Load saved theme preference
            saved_theme = self.config.get_setting('app-theme', 'default')
            theme_mapping = {'default': 0, 'light': 1, 'dark': 2}
            self.theme_row.set_selected(theme_mapping.get(saved_theme, 0))
            
            self.theme_row.connect('notify::selected', self.on_theme_changed)
            
            interface_appearance_group.add(self.theme_row)
            interface_page.add(interface_appearance_group)
            
            # Window group
            window_group = Adw.PreferencesGroup()
            window_group.set_title("Window")
            
            # Remember window size switch
            remember_size_switch = Adw.SwitchRow()
            remember_size_switch.set_title("Remember Window Size")
            remember_size_switch.set_subtitle("Restore window size on startup")
            remember_size_switch.set_active(True)
            
            # Auto focus terminal switch
            auto_focus_switch = Adw.SwitchRow()
            auto_focus_switch.set_title("Auto Focus Terminal")
            auto_focus_switch.set_subtitle("Focus terminal when connecting")
            auto_focus_switch.set_active(True)
            
            window_group.add(remember_size_switch)
            window_group.add(auto_focus_switch)
            interface_page.add(window_group)

            # Advanced SSH settings
            advanced_page = Adw.PreferencesPage()
            advanced_page.set_title("Advanced")
            advanced_page.set_icon_name("applications-system-symbolic")

            advanced_group = Adw.PreferencesGroup()
            advanced_group.set_title("SSH Settings")
            # Use custom options toggle
            self.apply_advanced_row = Adw.SwitchRow()
            self.apply_advanced_row.set_title("Use custom connection options")
            self.apply_advanced_row.set_subtitle("Enable and edit the options below")
            self.apply_advanced_row.set_active(bool(self.config.get_setting('ssh.apply_advanced', False)))
            advanced_group.add(self.apply_advanced_row)


            # Connect timeout
            self.connect_timeout_row = Adw.SpinRow.new_with_range(1, 120, 1)
            self.connect_timeout_row.set_title("Connect Timeout (s)")
            self.connect_timeout_row.set_value(self.config.get_setting('ssh.connection_timeout', 10))
            advanced_group.add(self.connect_timeout_row)

            # Connection attempts
            self.connection_attempts_row = Adw.SpinRow.new_with_range(1, 10, 1)
            self.connection_attempts_row.set_title("Connection Attempts")
            self.connection_attempts_row.set_value(self.config.get_setting('ssh.connection_attempts', 1))
            advanced_group.add(self.connection_attempts_row)

            # Keepalive interval
            self.keepalive_interval_row = Adw.SpinRow.new_with_range(0, 300, 5)
            self.keepalive_interval_row.set_title("ServerAlive Interval (s)")
            self.keepalive_interval_row.set_value(self.config.get_setting('ssh.keepalive_interval', 30))
            advanced_group.add(self.keepalive_interval_row)

            # Keepalive count max
            self.keepalive_count_row = Adw.SpinRow.new_with_range(1, 10, 1)
            self.keepalive_count_row.set_title("ServerAlive CountMax")
            self.keepalive_count_row.set_value(self.config.get_setting('ssh.keepalive_count_max', 3))
            advanced_group.add(self.keepalive_count_row)

            # Strict host key checking
            self.strict_host_row = Adw.ComboRow()
            self.strict_host_row.set_title("StrictHostKeyChecking")
            strict_model = Gtk.StringList()
            for item in ["accept-new", "yes", "no", "ask"]:
                strict_model.append(item)
            self.strict_host_row.set_model(strict_model)
            # Map current value
            current_strict = str(self.config.get_setting('ssh.strict_host_key_checking', 'accept-new'))
            try:
                idx = ["accept-new", "yes", "no", "ask"].index(current_strict)
            except ValueError:
                idx = 0
            self.strict_host_row.set_selected(idx)
            advanced_group.add(self.strict_host_row)

            # BatchMode (non-interactive)
            self.batch_mode_row = Adw.SwitchRow()
            self.batch_mode_row.set_title("BatchMode (disable prompts)")
            self.batch_mode_row.set_active(bool(self.config.get_setting('ssh.batch_mode', True)))
            advanced_group.add(self.batch_mode_row)

            # Compression
            self.compression_row = Adw.SwitchRow()
            self.compression_row.set_title("Enable Compression (-C)")
            self.compression_row.set_active(bool(self.config.get_setting('ssh.compression', True)))
            advanced_group.add(self.compression_row)

            # SSH verbosity (-v levels)
            self.verbosity_row = Adw.SpinRow.new_with_range(0, 3, 1)
            self.verbosity_row.set_title("SSH Verbosity (-v)")
            self.verbosity_row.set_value(int(self.config.get_setting('ssh.verbosity', 0)))
            advanced_group.add(self.verbosity_row)

            # Debug logging toggle
            self.debug_enabled_row = Adw.SwitchRow()
            self.debug_enabled_row.set_title("Enable SSH Debug Logging")
            self.debug_enabled_row.set_active(bool(self.config.get_setting('ssh.debug_enabled', False)))
            advanced_group.add(self.debug_enabled_row)

            # Reset button
            reset_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
            reset_btn = Gtk.Button.new_with_label("Reset Advanced SSH to Defaults")
            reset_btn.add_css_class('destructive-action')
            reset_btn.connect('clicked', self.on_reset_advanced_ssh)
            reset_box.append(reset_btn)
            advanced_group.add(reset_box)

            # Disable/enable advanced controls based on toggle
            def _sync_advanced_sensitivity(row=None, *_):
                enabled = bool(self.apply_advanced_row.get_active())
                for w in [self.connect_timeout_row, self.connection_attempts_row,
                          self.keepalive_interval_row, self.keepalive_count_row,
                          self.strict_host_row, self.batch_mode_row,
                          self.compression_row, self.verbosity_row,
                          self.debug_enabled_row]:
                    try:
                        w.set_sensitive(enabled)
                    except Exception:
                        pass
            _sync_advanced_sensitivity()
            self.apply_advanced_row.connect('notify::active', _sync_advanced_sensitivity)

            advanced_page.add(advanced_group)

            # Add pages to the preferences window
            self.add(terminal_page)
            self.add(interface_page)
            self.add(advanced_page)
            
            logger.info("Preferences window initialized")
        except Exception as e:
            logger.error(f"Failed to setup preferences: {e}")

    def on_close_request(self, *args):
        """Persist settings when the preferences window closes"""
        try:
            self.save_advanced_ssh_settings()
            # Ensure preferences are flushed to disk
            if hasattr(self.config, 'save_json_config'):
                self.config.save_json_config()
        except Exception:
            pass
        return False  # allow close
    
    def on_font_button_clicked(self, button):
        """Handle font button click"""
        logger.info("Font button clicked")
        
        # Create font chooser dialog
        font_dialog = Gtk.FontDialog()
        font_dialog.set_title("Choose Terminal Font (Monospace Recommended)")
        
        # Set current font (get from config or default)
        current_font = self.config.get_setting('terminal-font', 'Monospace 12')
        font_desc = Pango.FontDescription.from_string(current_font)
        
        def on_font_selected(dialog, result):
            try:
                font_desc = dialog.choose_font_finish(result)
                if font_desc:
                    font_string = font_desc.to_string()
                    self.font_row.set_subtitle(font_string)
                    logger.info(f"Font selected: {font_string}")
                    
                    # Save to config
                    self.config.set_setting('terminal-font', font_string)
                    
                    # Apply to all active terminals
                    self.apply_font_to_terminals(font_string)
                    
            except Exception as e:
                logger.warning(f"Font selection cancelled or failed: {e}")
        
        font_dialog.choose_font(self, None, None, on_font_selected)
    
    def apply_font_to_terminals(self, font_string):
        """Apply font to all active terminal widgets"""
        try:
            parent_window = self.get_transient_for()
            if parent_window and hasattr(parent_window, 'connection_to_terminals'):
                font_desc = Pango.FontDescription.from_string(font_string)
                count = 0
                for terms in parent_window.connection_to_terminals.values():
                    for terminal in terms:
                        if hasattr(terminal, 'vte'):
                            terminal.vte.set_font(font_desc)
                            count += 1
                logger.info(f"Applied font {font_string} to {count} terminals")
        except Exception as e:
            logger.error(f"Failed to apply font to terminals: {e}")
    
    def on_theme_changed(self, combo_row, param):
        """Handle theme selection change"""
        selected = combo_row.get_selected()
        theme_names = ["Follow System", "Light", "Dark"]
        selected_theme = theme_names[selected] if selected < len(theme_names) else "Follow System"
        
        logger.info(f"Theme changed to: {selected_theme}")
        
        # Apply theme immediately
        style_manager = Adw.StyleManager.get_default()
        
        if selected == 0:  # Follow System
            style_manager.set_color_scheme(Adw.ColorScheme.DEFAULT)
            self.config.set_setting('app-theme', 'default')
        elif selected == 1:  # Light
            style_manager.set_color_scheme(Adw.ColorScheme.FORCE_LIGHT)
            self.config.set_setting('app-theme', 'light')
        elif selected == 2:  # Dark
            style_manager.set_color_scheme(Adw.ColorScheme.FORCE_DARK)
            self.config.set_setting('app-theme', 'dark')

    def save_advanced_ssh_settings(self):
        """Persist advanced SSH settings from the preferences UI"""
        try:
            if hasattr(self, 'apply_advanced_row'):
                self.config.set_setting('ssh.apply_advanced', bool(self.apply_advanced_row.get_active()))
            if hasattr(self, 'connect_timeout_row'):
                self.config.set_setting('ssh.connection_timeout', int(self.connect_timeout_row.get_value()))
            if hasattr(self, 'connection_attempts_row'):
                self.config.set_setting('ssh.connection_attempts', int(self.connection_attempts_row.get_value()))
            if hasattr(self, 'keepalive_interval_row'):
                self.config.set_setting('ssh.keepalive_interval', int(self.keepalive_interval_row.get_value()))
            if hasattr(self, 'keepalive_count_row'):
                self.config.set_setting('ssh.keepalive_count_max', int(self.keepalive_count_row.get_value()))
            if hasattr(self, 'strict_host_row'):
                options = ["accept-new", "yes", "no", "ask"]
                idx = self.strict_host_row.get_selected()
                value = options[idx] if 0 <= idx < len(options) else 'accept-new'
                self.config.set_setting('ssh.strict_host_key_checking', value)
            if hasattr(self, 'batch_mode_row'):
                self.config.set_setting('ssh.batch_mode', bool(self.batch_mode_row.get_active()))
            if hasattr(self, 'compression_row'):
                self.config.set_setting('ssh.compression', bool(self.compression_row.get_active()))
            if hasattr(self, 'verbosity_row'):
                self.config.set_setting('ssh.verbosity', int(self.verbosity_row.get_value()))
            if hasattr(self, 'debug_enabled_row'):
                self.config.set_setting('ssh.debug_enabled', bool(self.debug_enabled_row.get_active()))
        except Exception as e:
            logger.error(f"Failed to save advanced SSH settings: {e}")

    def on_reset_advanced_ssh(self, *_args):
        """Reset only advanced SSH keys to defaults and update UI."""
        try:
            defaults = self.config.get_default_config().get('ssh', {})
            # Persist defaults and disable apply
            self.config.set_setting('ssh.apply_advanced', False)
            for key in ['connection_timeout', 'connection_attempts', 'keepalive_interval', 'keepalive_count_max', 'compression', 'auto_add_host_keys', 'verbosity', 'debug_enabled']:
                self.config.set_setting(f'ssh.{key}', defaults.get(key))
            # Update UI
            if hasattr(self, 'apply_advanced_row'):
                self.apply_advanced_row.set_active(False)
            if hasattr(self, 'connect_timeout_row'):
                self.connect_timeout_row.set_value(int(defaults.get('connection_timeout', 30)))
            if hasattr(self, 'connection_attempts_row'):
                self.connection_attempts_row.set_value(int(defaults.get('connection_attempts', 1)))
            if hasattr(self, 'keepalive_interval_row'):
                self.keepalive_interval_row.set_value(int(defaults.get('keepalive_interval', 60)))
            if hasattr(self, 'keepalive_count_row'):
                self.keepalive_count_row.set_value(int(defaults.get('keepalive_count_max', 3)))
            if hasattr(self, 'strict_host_row'):
                try:
                    self.strict_host_row.set_selected(["accept-new", "yes", "no", "ask"].index('accept-new'))
                except ValueError:
                    self.strict_host_row.set_selected(0)
            if hasattr(self, 'batch_mode_row'):
                self.batch_mode_row.set_active(False)
            if hasattr(self, 'compression_row'):
                self.compression_row.set_active(bool(defaults.get('compression', True)))
            if hasattr(self, 'verbosity_row'):
                self.verbosity_row.set_value(int(defaults.get('verbosity', 0)))
            if hasattr(self, 'debug_enabled_row'):
                self.debug_enabled_row.set_active(bool(defaults.get('debug_enabled', False)))
        except Exception as e:
            logger.error(f"Failed to reset advanced SSH settings: {e}")
    
    def get_theme_name_mapping(self):
        """Get mapping between display names and config keys"""
        return {
            "Default": "default",
            "Solarized Dark": "solarized_dark", 
            "Solarized Light": "solarized_light",
            "Monokai": "monokai",
            "Dracula": "dracula",
            "Nord": "nord",
            "Gruvbox Dark": "gruvbox_dark",
            "One Dark": "one_dark",
            "Tomorrow Night": "tomorrow_night",
            "Material Dark": "material_dark",
        }
    
    def get_reverse_theme_mapping(self):
        """Get mapping from config keys to display names"""
        mapping = self.get_theme_name_mapping()
        return {v: k for k, v in mapping.items()}
    
    def on_color_scheme_changed(self, combo_row, param):
        """Handle terminal color scheme change"""
        selected = combo_row.get_selected()
        scheme_names = [
            "Default", "Solarized Dark", "Solarized Light",
            "Monokai", "Dracula", "Nord",
            "Gruvbox Dark", "One Dark", "Tomorrow Night", "Material Dark"
        ]
        selected_scheme = scheme_names[selected] if selected < len(scheme_names) else "Default"
        
        logger.info(f"Terminal color scheme changed to: {selected_scheme}")
        
        # Convert display name to config key
        theme_mapping = self.get_theme_name_mapping()
        config_key = theme_mapping.get(selected_scheme, "default")
        
        # Save to config using the consistent key
        self.config.set_setting('terminal.theme', config_key)
        
        # Apply to all active terminals
        self.apply_color_scheme_to_terminals(config_key)
        
    def on_confirm_disconnect_changed(self, switch, *args):
        """Handle confirm disconnect setting change"""
        confirm = switch.get_active()
        logger.info(f"Confirm before disconnect setting changed to: {confirm}")
        self.config.set_setting('confirm-disconnect', confirm)
    
    def apply_color_scheme_to_terminals(self, scheme_key):
        """Apply color scheme to all active terminal widgets"""
        try:
            parent_window = self.get_transient_for()
            if parent_window and hasattr(parent_window, 'connection_to_terminals'):
                count = 0
                for terms in parent_window.connection_to_terminals.values():
                    for terminal in terms:
                        if hasattr(terminal, 'apply_theme'):
                            terminal.apply_theme(scheme_key)
                            count += 1
                logger.info(f"Applied color scheme {scheme_key} to {count} terminals")
        except Exception as e:
            logger.error(f"Failed to apply color scheme to terminals: {e}")

class MainWindow(Adw.ApplicationWindow):
    """Main application window"""
    
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.active_terminals = {}
        self.connections = []
        self._is_quitting = False  # Flag to prevent multiple quit attempts
        self._is_controlled_reconnect = False  # Flag to track controlled reconnection
        
        # Initialize managers
        self.connection_manager = ConnectionManager()
        self.config = Config()
        self.key_manager = KeyManager()
        
        # UI state
        self.active_terminals: Dict[Connection, TerminalWidget] = {}  # most recent terminal per connection
        self.connection_to_terminals: Dict[Connection, List[TerminalWidget]] = {}
        self.terminal_to_connection: Dict[TerminalWidget, Connection] = {}
        self.connection_rows = {}   # connection -> row_widget
        # Hide hosts toggle state
        try:
            self._hide_hosts = bool(self.config.get_setting('ui.hide_hosts', False))
        except Exception:
            self._hide_hosts = False
        
        # Set up window
        self.setup_window()
        self.setup_ui()
        self.setup_connections()
        self.setup_signals()
        
        # Add action for activating connections
        self.activate_action = Gio.SimpleAction.new('activate-connection', None)
        self.activate_action.connect('activate', self.on_activate_connection)
        self.add_action(self.activate_action)
        # Context menu action to force opening a new connection tab
        self.open_new_connection_action = Gio.SimpleAction.new('open-new-connection', None)
        self.open_new_connection_action.connect('activate', self.on_open_new_connection_action)
        self.add_action(self.open_new_connection_action)
        # (Toasts disabled) Remove any toast-related actions if previously defined
        try:
            if hasattr(self, '_toast_reconnect_action'):
                self.remove_action('toast-reconnect')
        except Exception:
            pass
        
        # Connect to close request signal
        self.connect('close-request', self.on_close_request)
        
        # Start with welcome view (tab view setup already shows welcome initially)
        
        logger.info("Main window initialized")

        # On startup, focus the first item in the connection list (not the toolbar buttons)
        try:
            GLib.idle_add(self._focus_connection_list_first_row)
        except Exception:
            pass

    def setup_window(self):
        """Configure main window properties"""
        self.set_title('sshPilot')
        self.set_icon_name('io.github.mfat.sshpilot')
        
        # Load window geometry
        geometry = self.config.get_window_geometry()
        self.set_default_size(geometry['width'], geometry['height'])
        
        # Connect window state signals
        self.connect('notify::default-width', self.on_window_size_changed)
        self.connect('notify::default-height', self.on_window_size_changed)
        # Ensure initial focus after the window is mapped
        try:
            self.connect('map', lambda *a: GLib.timeout_add(50, self._focus_connection_list_first_row))
        except Exception:
            pass

        # Global shortcuts for tab navigation: Alt+Right / Alt+Left
        try:
            nav = Gtk.ShortcutController()
            nav.set_scope(Gtk.ShortcutScope.GLOBAL)
            if hasattr(nav, 'set_propagation_phase'):
                nav.set_propagation_phase(Gtk.PropagationPhase.BUBBLE)

            def _cb_next(widget, *args):
                try:
                    self._select_tab_relative(1)
                except Exception:
                    pass
                return True

            def _cb_prev(widget, *args):
                try:
                    self._select_tab_relative(-1)
                except Exception:
                    pass
                return True

            nav.add_shortcut(Gtk.Shortcut.new(
                Gtk.ShortcutTrigger.parse_string('<Alt>Right'),
                Gtk.CallbackAction.new(_cb_next)
            ))
            nav.add_shortcut(Gtk.Shortcut.new(
                Gtk.ShortcutTrigger.parse_string('<Alt>Left'),
                Gtk.CallbackAction.new(_cb_prev)
            ))
            self.add_controller(nav)
        except Exception:
            pass
        
    def on_window_size_changed(self, window, param):
        """Handle window size changes and save the new dimensions"""
        width = self.get_default_width()
        height = self.get_default_height()
        logger.debug(f"Window size changed to: {width}x{height}")
        
        # Save the new window geometry
        self.config.set_window_geometry(width, height)

    def setup_ui(self):
        """Set up the user interface"""
        # Create main container
        main_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        
        # Create header bar
        self.header_bar = Adw.HeaderBar()
        self.header_bar.set_title_widget(Gtk.Label(label="sshPilot"))
        
        # Add window controls (minimize, maximize, close)
        self.header_bar.set_show_start_title_buttons(True)
        self.header_bar.set_show_end_title_buttons(True)
        
        # Add header bar to main container
        main_box.append(self.header_bar)
        
        # Create main layout (fallback if OverlaySplitView is unavailable)
        if HAS_OVERLAY_SPLIT:
            self.split_view = Adw.OverlaySplitView()
            try:
                self.split_view.set_sidebar_width_fraction(0.25)
                self.split_view.set_min_sidebar_width(200)
                self.split_view.set_max_sidebar_width(400)
            except Exception:
                pass
            self.split_view.set_vexpand(True)
            self._split_variant = 'overlay'
        else:
            self.split_view = Gtk.Paned.new(Gtk.Orientation.HORIZONTAL)
            self.split_view.set_wide_handle(True)
            self.split_view.set_vexpand(True)
            self._split_variant = 'paned'
        
        # Create sidebar
        self.setup_sidebar()
        
        # Create main content area
        self.setup_content_area()
        
        # Add split view to main container
        main_box.append(self.split_view)

        # Set main content (no toasts preferred)
        self.set_content(main_box)

    def _set_sidebar_widget(self, widget: Gtk.Widget) -> None:
        if HAS_OVERLAY_SPLIT:
            try:
                self.split_view.set_sidebar(widget)
                return
            except Exception:
                pass
        # Fallback for Gtk.Paned
        try:
            self.split_view.set_start_child(widget)
        except Exception:
            pass

    def _set_content_widget(self, widget: Gtk.Widget) -> None:
        if HAS_OVERLAY_SPLIT:
            try:
                self.split_view.set_content(widget)
                return
            except Exception:
                pass
        # Fallback for Gtk.Paned
        try:
            self.split_view.set_end_child(widget)
        except Exception:
            pass

    def _get_sidebar_width(self) -> int:
        try:
            if HAS_OVERLAY_SPLIT and hasattr(self.split_view, 'get_max_sidebar_width'):
                return int(self.split_view.get_max_sidebar_width())
        except Exception:
            pass
        # Fallback: attempt to read allocation of the first child when using Paned
        try:
            sidebar = self.split_view.get_start_child()
            if sidebar is not None:
                alloc = sidebar.get_allocation()
                return int(alloc.width)
        except Exception:
            pass
        return 0

    def setup_sidebar(self):
        """Set up the sidebar with connection list"""
        sidebar_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        
        # Sidebar header
        header = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        header.set_margin_start(12)
        header.set_margin_end(12)
        header.set_margin_top(12)
        header.set_margin_bottom(6)
        
        # Title
        title_label = Gtk.Label()
        title_label.set_markup('<b>Connections</b>')
        title_label.set_halign(Gtk.Align.START)
        title_label.set_hexpand(True)
        header.append(title_label)
        
        # Add connection button
        add_button = Gtk.Button.new_from_icon_name('list-add-symbolic')
        add_button.set_tooltip_text('Add Connection (Ctrl+N)')
        add_button.connect('clicked', self.on_add_connection_clicked)
        try:
            add_button.set_can_focus(False)
        except Exception:
            pass
        header.append(add_button)

        # Hide/Show hostnames button (eye icon)
        def _update_eye_icon(btn):
            try:
                icon = 'view-conceal-symbolic' if self._hide_hosts else 'view-reveal-symbolic'
                btn.set_icon_name(icon)
                btn.set_tooltip_text('Show hostnames' if self._hide_hosts else 'Hide hostnames')
            except Exception:
                pass

        hide_button = Gtk.Button.new_from_icon_name('view-reveal-symbolic')
        _update_eye_icon(hide_button)
        def _on_toggle_hide(btn):
            try:
                self._hide_hosts = not self._hide_hosts
                # Persist setting
                try:
                    self.config.set_setting('ui.hide_hosts', self._hide_hosts)
                except Exception:
                    pass
                # Update all rows
                for row in self.connection_rows.values():
                    if hasattr(row, 'apply_hide_hosts'):
                        row.apply_hide_hosts(self._hide_hosts)
                # Update icon/tooltip
                _update_eye_icon(btn)
            except Exception:
                pass
        hide_button.connect('clicked', _on_toggle_hide)
        try:
            hide_button.set_can_focus(False)
        except Exception:
            pass
        header.append(hide_button)
        
        sidebar_box.append(header)
        
        # Connection list
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        scrolled.set_vexpand(True)
        
        self.connection_list = Gtk.ListBox()
        self.connection_list.set_selection_mode(Gtk.SelectionMode.SINGLE)
        try:
            self.connection_list.set_can_focus(True)
        except Exception:
            pass
        
        # Connect signals
        self.connection_list.connect('row-selected', self.on_connection_selected)  # For button sensitivity
        self.connection_list.connect('row-activated', self.on_connection_activated)  # For Enter key/double-click
        
        # Make sure the connection list is focusable and can receive key events
        self.connection_list.set_focusable(True)
        self.connection_list.set_can_focus(True)
        self.connection_list.set_focus_on_click(True)
        self.connection_list.set_activate_on_single_click(False)  # Require double-click to activate
        
        # Set up drag and drop for reordering
        self.setup_connection_list_dnd()

        # Right-click context menu to open multiple connections
        try:
            context_click = Gtk.GestureClick()
            context_click.set_button(0)  # handle any button; filter inside
            def _on_list_pressed(gesture, n_press, x, y):
                try:
                    btn = 0
                    try:
                        btn = gesture.get_current_button()
                    except Exception:
                        pass
                    if btn not in (Gdk.BUTTON_SECONDARY, 3):
                        return
                    row = self.connection_list.get_row_at_y(int(y))
                    if not row:
                        return
                    self.connection_list.select_row(row)
                    self._context_menu_connection = getattr(row, 'connection', None)
                    menu = Gio.Menu()
                    menu.append(_('Open New Connection'), 'win.open-new-connection')
                    pop = Gtk.PopoverMenu.new_from_model(menu)
                    pop.set_parent(self.connection_list)
                    try:
                        rect = Gdk.Rectangle()
                        rect.x = int(x)
                        rect.y = int(y)
                        rect.width = 1
                        rect.height = 1
                        pop.set_pointing_to(rect)
                    except Exception:
                        pass
                    pop.popup()
                except Exception:
                    pass
            context_click.connect('pressed', _on_list_pressed)
            self.connection_list.add_controller(context_click)
        except Exception:
            pass
        
        scrolled.set_child(self.connection_list)
        sidebar_box.append(scrolled)
        
        # Sidebar toolbar
        toolbar = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        toolbar.set_margin_start(6)
        toolbar.set_margin_end(6)
        toolbar.set_margin_top(6)
        toolbar.set_margin_bottom(6)
        toolbar.add_css_class('toolbar')
        try:
            # Expose the computed visual height so terminal banners can match
            min_h, nat_h, min_baseline, nat_baseline = toolbar.measure(Gtk.Orientation.VERTICAL, -1)
            self._toolbar_row_height = max(min_h, nat_h)
            # Also track the real allocated height dynamically
            def _on_toolbar_alloc(widget, allocation):
                try:
                    self._toolbar_row_height = allocation.height
                except Exception:
                    pass
            toolbar.connect('size-allocate', _on_toolbar_alloc)
        except Exception:
            self._toolbar_row_height = 36
        
        # Edit button
        self.edit_button = Gtk.Button.new_from_icon_name('document-edit-symbolic')
        self.edit_button.set_tooltip_text('Edit Connection')
        self.edit_button.set_sensitive(False)
        self.edit_button.connect('clicked', self.on_edit_connection_clicked)
        toolbar.append(self.edit_button)

        # Copy key to server button (ssh-copy-id)
        self.copy_key_button = Gtk.Button.new_from_icon_name('dialog-password-symbolic')
        self.copy_key_button.set_tooltip_text('Copy public key to server for passwordless login')
        self.copy_key_button.set_sensitive(False)
        self.copy_key_button.connect('clicked', self.on_copy_key_to_server_clicked)
        toolbar.append(self.copy_key_button)

        # Upload (scp) button
        self.upload_button = Gtk.Button.new_from_icon_name('document-send-symbolic')
        self.upload_button.set_tooltip_text('Upload file(s) to server (scp)')
        self.upload_button.set_sensitive(False)
        self.upload_button.connect('clicked', self.on_upload_file_clicked)
        toolbar.append(self.upload_button)
        
        # Delete button
        self.delete_button = Gtk.Button.new_from_icon_name('user-trash-symbolic')
        self.delete_button.set_tooltip_text('Delete Connection')
        self.delete_button.set_sensitive(False)
        self.delete_button.connect('clicked', self.on_delete_connection_clicked)
        toolbar.append(self.delete_button)
        
        # Spacer
        spacer = Gtk.Box()
        spacer.set_hexpand(True)
        toolbar.append(spacer)
        
        # Menu button
        menu_button = Gtk.MenuButton()
        menu_button.set_icon_name('open-menu-symbolic')
        menu_button.set_tooltip_text('Menu')
        menu_button.set_menu_model(self.create_menu())
        toolbar.append(menu_button)
        
        sidebar_box.append(toolbar)
        
        self._set_sidebar_widget(sidebar_box)

    def setup_content_area(self):
        """Set up the main content area with stack for tabs and welcome view"""
        # Create stack to switch between welcome view and tab view
        self.content_stack = Gtk.Stack()
        self.content_stack.set_hexpand(True)
        self.content_stack.set_vexpand(True)
        
        # Create welcome/help view
        self.welcome_view = WelcomePage()
        self.content_stack.add_named(self.welcome_view, "welcome")
        
        # Create tab view
        self.tab_view = Adw.TabView()
        self.tab_view.set_hexpand(True)
        self.tab_view.set_vexpand(True)
        
        # Connect tab signals
        self.tab_view.connect('close-page', self.on_tab_close)
        self.tab_view.connect('page-attached', self.on_tab_attached)
        self.tab_view.connect('page-detached', self.on_tab_detached)

        # Whenever the window layout changes, propagate toolbar height to
        # any TerminalWidget so the reconnect banner exactly matches.
        try:
            # Capture the toolbar variable from this scope for measurement
            local_toolbar = locals().get('toolbar', None)
            def _sync_banner_heights(*_args):
                try:
                    # Re-measure toolbar height in case style/theme changed
                    try:
                        if local_toolbar is not None:
                            min_h, nat_h, min_baseline, nat_baseline = local_toolbar.measure(Gtk.Orientation.VERTICAL, -1)
                            self._toolbar_row_height = max(min_h, nat_h)
                    except Exception:
                        pass
                    # Push exact allocated height to all terminal widgets (+5px)
                    for terms in self.connection_to_terminals.values():
                        for term in terms:
                            if hasattr(term, 'set_banner_height'):
                                term.set_banner_height(getattr(self, '_toolbar_row_height', 37) + 55)
                except Exception:
                    pass
            # Call once after UI is built and again after a short delay
            def _push_now():
                try:
                    height = getattr(self, '_toolbar_row_height', 36)
                    for terms in self.connection_to_terminals.values():
                        for term in terms:
                            if hasattr(term, 'set_banner_height'):
                                term.set_banner_height(height + 55)
                except Exception:
                    pass
                return False
            GLib.idle_add(_sync_banner_heights)
            GLib.timeout_add(200, _sync_banner_heights)
            GLib.idle_add(_push_now)
        except Exception:
            pass
        
        # Create tab bar
        self.tab_bar = Adw.TabBar()
        self.tab_bar.set_view(self.tab_view)
        self.tab_bar.set_autohide(False)
        
        # Create tab content box
        tab_content_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        tab_content_box.append(self.tab_bar)
        tab_content_box.append(self.tab_view)
        # Ensure background matches terminal theme to avoid white flash
        if hasattr(tab_content_box, 'add_css_class'):
            tab_content_box.add_css_class('terminal-bg')
        
        self.content_stack.add_named(tab_content_box, "tabs")
        # Also color the stack background
        if hasattr(self.content_stack, 'add_css_class'):
            self.content_stack.add_css_class('terminal-bg')
        
        # Start with welcome view visible
        self.content_stack.set_visible_child_name("welcome")
        
        self._set_content_widget(self.content_stack)

    def setup_connection_list_dnd(self):
        """Set up drag and drop for connection list reordering"""
        # TODO: Implement drag and drop reordering
        pass

    def create_menu(self):
        """Create application menu"""
        menu = Gio.Menu()
        
        # Add all menu items directly to the main menu
        menu.append('New Connection', 'app.new-connection')
        menu.append('Generate SSH Key', 'app.new-key')
        menu.append('Preferences', 'app.preferences')
        menu.append('About', 'app.about')
        menu.append('Quit', 'app.quit')
        
        return menu

    def setup_connections(self):
        """Load and display existing connections"""
        connections = self.connection_manager.get_connections()
        
        for connection in connections:
            self.add_connection_row(connection)
        
        # Select first connection if available
        if connections:
            first_row = self.connection_list.get_row_at_index(0)
            if first_row:
                self.connection_list.select_row(first_row)
                # Defer focus to the list to ensure keyboard navigation works immediately
                GLib.idle_add(self._focus_connection_list_first_row)

    def setup_signals(self):
        """Connect to manager signals"""
        # Connection manager signals - use connect_after to avoid conflict with GObject.connect
        self.connection_manager.connect_after('connection-added', self.on_connection_added)
        self.connection_manager.connect_after('connection-removed', self.on_connection_removed)
        self.connection_manager.connect_after('connection-status-changed', self.on_connection_status_changed)
        
        # Config signals
        self.config.connect('setting-changed', self.on_setting_changed)

    def add_connection_row(self, connection: Connection):
        """Add a connection row to the list"""
        row = ConnectionRow(connection)
        self.connection_list.append(row)
        self.connection_rows[connection] = row
        # Apply current hide-hosts setting to new row
        if hasattr(row, 'apply_hide_hosts'):
            row.apply_hide_hosts(getattr(self, '_hide_hosts', False))

    def show_welcome_view(self):
        """Show the welcome/help view when no connections are active"""
        # Remove terminal background styling so welcome uses app theme colors
        if hasattr(self.content_stack, 'remove_css_class'):
            try:
                self.content_stack.remove_css_class('terminal-bg')
            except Exception:
                pass
        # Ensure welcome fills the pane
        if hasattr(self, 'welcome_view'):
            try:
                self.welcome_view.set_hexpand(True)
                self.welcome_view.set_vexpand(True)
            except Exception:
                pass
        self.content_stack.set_visible_child_name("welcome")
        logger.info("Showing welcome view")

    def _focus_connection_list_first_row(self):
        """Focus the connection list and ensure the first row is selected."""
        try:
            if not hasattr(self, 'connection_list') or self.connection_list is None:
                return False
            # If the list has no selection, select the first row
            selected = self.connection_list.get_selected_row() if hasattr(self.connection_list, 'get_selected_row') else None
            first_row = self.connection_list.get_row_at_index(0)
            if not selected and first_row:
                self.connection_list.select_row(first_row)
            # If no widget currently has focus in the window, give it to the list
            focus_widget = self.get_focus() if hasattr(self, 'get_focus') else None
            if focus_widget is None and first_row:
                self.connection_list.grab_focus()
        except Exception:
            pass
        return False
    
    def show_tab_view(self):
        """Show the tab view when connections are active"""
        # Re-apply terminal background when switching back to tabs
        if hasattr(self.content_stack, 'add_css_class'):
            try:
                self.content_stack.add_css_class('terminal-bg')
            except Exception:
                pass
        self.content_stack.set_visible_child_name("tabs")
        logger.info("Showing tab view")

    def show_connection_dialog(self, connection: Connection = None):
        """Show connection dialog for adding/editing connections"""
        logger.info(f"Show connection dialog for: {connection}")
        
        # Create connection dialog
        dialog = ConnectionDialog(self, connection)
        dialog.connect('connection-saved', self.on_connection_saved)
        dialog.present()

    def show_key_dialog(self):
        """Show SSH key generation dialog"""
        try:
            dialog = Adw.MessageDialog(
                transient_for=self,
                modal=True,
                heading=_("Generate SSH Key Pair"),
                body=_("We will create a private key and its matching public key (.pub). Choose how to generate the key pair:")
            )
            dialog.add_response('cancel', _("Cancel"))
            dialog.add_response('builtin', _("Generate (Built-in)"))
            dialog.add_response('ssh-keygen', _("Generate (ssh-keygen)"))
            dialog.set_default_response('builtin')
            dialog.set_close_response('cancel')

            def _on_resp(dlg, response):
                dlg.close()
                if response in ('builtin', 'ssh-keygen'):
                    self._present_key_generator(response)

            dialog.connect('response', _on_resp)
            dialog.present()
        except Exception as e:
            logger.error(f"Failed to show key dialog: {e}")

    def _present_key_generator(self, method: str):
        """Present a minimal key generation UI and generate the key."""
        try:
            # Simple inline generator dialog
            gen = Adw.MessageDialog(
                transient_for=self,
                modal=True,
                heading=_("New SSH Key Pair"),
                body=_("Enter a file name and choose a key type. We will generate both private and public keys.")
            )
            # Inputs
            box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
            name_row = Adw.EntryRow(title=_("Key name"))
            name_row.set_text("id_ed25519")
            # Use a simple DropDown so both options are clearly visible
            type_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
            type_label = Gtk.Label(label=_("Key type"))
            type_label.set_halign(Gtk.Align.START)
            type_label.set_hexpand(True)
            key_type_dropdown = Gtk.DropDown.new_from_strings(["ed25519", "rsa"])
            key_type_dropdown.set_selected(0)
            type_box.append(type_label)
            type_box.append(key_type_dropdown)
            box.append(name_row)
            box.append(type_box)
            gen.set_extra_child(box)
            gen.add_response('cancel', _("Cancel"))
            gen.add_response('generate', _("Generate"))
            gen.set_default_response('generate')
            gen.set_close_response('cancel')

            def _on_gen(dlg, response):
                dlg.close()
                if response != 'generate':
                    return
                key_name = name_row.get_text().strip() or 'id_ed25519'
                key_type = ['ed25519', 'rsa'][key_type_dropdown.get_selected()]
                try:
                    if method == 'ssh-keygen':
                        ssh_key = self.key_manager.generate_key_with_ssh_keygen(
                            key_name=key_name,
                            key_type=key_type,
                            key_size=4096 if key_type == 'rsa' else 0,
                            comment=f"{os.getenv('USER')}@{os.uname().nodename}"
                        )
                    else:
                        ssh_key = self.key_manager.generate_key(
                            key_name=key_name,
                            key_type=key_type,
                            key_size=4096 if key_type == 'rsa' else 0,
                            comment=f"{os.getenv('USER')}@{os.uname().nodename}"
                        )
                    if ssh_key:
                        dlg = Adw.MessageDialog(
                            transient_for=self,
                            modal=True,
                            heading=_("Success"),
                            body=_("SSH key pair generated:\nPrivate: {}\nPublic: {}.pub").format(ssh_key.path, ssh_key.path)
                        )
                        dlg.add_response('ok', _("OK"))
                        dlg.set_default_response('ok')
                        dlg.set_close_response('ok')
                        dlg.present()
                    else:
                        dlg = Adw.MessageDialog(
                            transient_for=self,
                            modal=True,
                            heading=_("Error"),
                            body=_("Failed to generate SSH key pair")
                        )
                        dlg.add_response('ok', _("OK"))
                        dlg.set_default_response('ok')
                        dlg.set_close_response('ok')
                        dlg.present()
                except Exception as e:
                    logger.error(f"Key generation failed: {e}")
                    dlg = Adw.MessageDialog(
                        transient_for=self,
                        modal=True,
                        heading=_("Error"),
                        body=str(e)
                    )
                    dlg.add_response('ok', _("OK"))
                    dlg.set_default_response('ok')
                    dlg.set_close_response('ok')
                    dlg.present()

            gen.connect('response', _on_gen)
            gen.present()
        except Exception as e:
            logger.error(f"Failed to present key generator: {e}")

    def show_preferences(self):
        """Show preferences dialog"""
        logger.info("Show preferences dialog")
        try:
            preferences_window = PreferencesWindow(self, self.config)
            preferences_window.present()
        except Exception as e:
            logger.error(f"Failed to show preferences dialog: {e}")

    def show_about_dialog(self):
        """Show about dialog"""
        # Use Gtk.AboutDialog so we can force a logo even without icon theme entries
        about = Gtk.AboutDialog()
        about.set_transient_for(self)
        about.set_modal(True)
        about.set_program_name('sshPilot')
        try:
            from . import __version__ as APP_VERSION
        except Exception:
            APP_VERSION = "0.0.0"
        about.set_version(APP_VERSION)
        about.set_comments('SSH connection manager with integrated terminal')
        about.set_website('https://github.com/mfat/sshpilot')
        # Gtk.AboutDialog in GTK4 has no set_issue_url; include issue link in website label
        about.set_website_label('Project homepage')
        about.set_license_type(Gtk.License.GPL_3_0)
        about.set_authors(['mFat <newmfat@gmail.com>'])
        
        # Attempt to load logo from GResource; fall back to local files
        logo_texture = None
        # 1) From GResource bundle
        for resource_path in (
            '/io/github/mfat/sshpilot/sshpilot.svg',
        ):
            try:
                logo_texture = Gdk.Texture.new_from_resource(resource_path)
                if logo_texture:
                    break
            except Exception:
                logo_texture = None
        # 2) From project-local files
        if logo_texture is None:
            candidate_files = []
            # repo root (user added io.github.mfat.sshpilot.png)
            try:
                path = os.path.abspath(os.path.dirname(__file__))
                repo_root = path
                while True:
                    if os.path.exists(os.path.join(repo_root, '.git')):
                        break
                    parent = os.path.dirname(repo_root)
                    if parent == repo_root:
                        break
                    repo_root = parent
                candidate_files.extend([
                    os.path.join(repo_root, 'io.github.mfat.sshpilot.svg'),
                    os.path.join(repo_root, 'sshpilot.svg'),
                ])
                # package resources folder (when running from source)
                candidate_files.append(os.path.join(os.path.dirname(__file__), 'resources', 'sshpilot.svg'))
            except Exception:
                pass
            for png_path in candidate_files:
                try:
                    if os.path.exists(png_path):
                        logo_texture = Gdk.Texture.new_from_filename(png_path)
                        if logo_texture:
                            break
                except Exception:
                    logo_texture = None
        # Apply if loaded
        if logo_texture is not None:
            try:
                about.set_logo(logo_texture)
            except Exception:
                pass
        
        about.present()

    def toggle_list_focus(self):
        """Toggle focus between connection list and terminal"""
        if self.connection_list.has_focus():
            # Focus current terminal
            current_page = self.tab_view.get_selected_page()
            if current_page:
                child = current_page.get_child()
                if hasattr(child, 'vte'):
                    child.vte.grab_focus()
        else:
            # Focus connection list
            self.connection_list.grab_focus()

    def _select_tab_relative(self, delta: int):
        """Select tab relative to current index, wrapping around."""
        try:
            n = self.tab_view.get_n_pages()
            if n <= 0:
                return
            current = self.tab_view.get_selected_page()
            # If no current selection, pick first
            if not current:
                page = self.tab_view.get_nth_page(0)
                if page:
                    self.tab_view.set_selected_page(page)
                return
            # Find current index
            idx = 0
            for i in range(n):
                if self.tab_view.get_nth_page(i) == current:
                    idx = i
                    break
            new_index = (idx + delta) % n
            page = self.tab_view.get_nth_page(new_index)
            if page:
                self.tab_view.set_selected_page(page)
        except Exception:
            pass

    def connect_to_host(self, connection: Connection, force_new: bool = False):
        """Connect to SSH host and create terminal tab.
        If force_new is False and a tab exists for this server, select the most recent tab.
        If force_new is True, always open a new tab.
        """
        if not force_new:
            # If a tab exists for this connection, activate the most recent one
            if connection in self.active_terminals:
                terminal = self.active_terminals[connection]
                page = self.tab_view.get_page(terminal)
                if page is not None:
                    self.tab_view.set_selected_page(page)
                    return
                else:
                    # Terminal exists but not in tab view, remove from active terminals
                    logger.warning(f"Terminal for {connection.nickname} not found in tab view, removing from active terminals")
                    del self.active_terminals[connection]
            # Fallback: look up any existing terminals for this connection
            existing_terms = self.connection_to_terminals.get(connection) or []
            for t in reversed(existing_terms):  # most recent last
                page = self.tab_view.get_page(t)
                if page is not None:
                    self.active_terminals[connection] = t
                    self.tab_view.set_selected_page(page)
                    return
        
        # Create new terminal
        terminal = TerminalWidget(connection, self.config, self.connection_manager)
        
        # Connect signals
        terminal.connect('connection-established', self.on_terminal_connected)
        terminal.connect('connection-failed', lambda w, e: logger.error(f"Connection failed: {e}"))
        terminal.connect('connection-lost', self.on_terminal_disconnected)
        terminal.connect('title-changed', self.on_terminal_title_changed)
        
        # Add to tab view
        page = self.tab_view.append(terminal)
        page.set_title(connection.nickname)
        page.set_icon(Gio.ThemedIcon.new('utilities-terminal-symbolic'))
        
        # Store references for multi-tab tracking
        self.connection_to_terminals.setdefault(connection, []).append(terminal)
        self.terminal_to_connection[terminal] = connection
        self.active_terminals[connection] = terminal
        
        # Switch to tab view when first connection is made
        self.show_tab_view()
        
        # Activate the new tab
        self.tab_view.set_selected_page(page)
        
        # Force set colors after the terminal is added to the UI
        def _set_terminal_colors():
            try:
                # Set colors using RGBA
                fg = Gdk.RGBA()
                fg.parse('rgb(0,0,0)')  # Black
                
                bg = Gdk.RGBA()
                bg.parse('rgb(255,255,255)')  # White
                
                # Set colors using both methods for maximum compatibility
                terminal.vte.set_color_foreground(fg)
                terminal.vte.set_color_background(bg)
                terminal.vte.set_colors(fg, bg, None)
                
                # Force a redraw
                terminal.vte.queue_draw()
                
                # Connect to the SSH server after setting colors
                if not terminal._connect_ssh():
                    logger.error("Failed to establish SSH connection")
                    self.tab_view.close_page(page)
                    # Cleanup on failure
                    try:
                        if connection in self.active_terminals and self.active_terminals[connection] is terminal:
                            del self.active_terminals[connection]
                        if terminal in self.terminal_to_connection:
                            del self.terminal_to_connection[terminal]
                        if connection in self.connection_to_terminals and terminal in self.connection_to_terminals[connection]:
                            self.connection_to_terminals[connection].remove(terminal)
                            if not self.connection_to_terminals[connection]:
                                del self.connection_to_terminals[connection]
                    except Exception:
                        pass
                        
            except Exception as e:
                logger.error(f"Error setting terminal colors: {e}")
                # Still try to connect even if color setting fails
                if not terminal._connect_ssh():
                    logger.error("Failed to establish SSH connection")
                    self.tab_view.close_page(page)
                    # Cleanup on failure
                    try:
                        if connection in self.active_terminals and self.active_terminals[connection] is terminal:
                            del self.active_terminals[connection]
                        if terminal in self.terminal_to_connection:
                            del self.terminal_to_connection[terminal]
                        if connection in self.connection_to_terminals and terminal in self.connection_to_terminals[connection]:
                            self.connection_to_terminals[connection].remove(terminal)
                            if not self.connection_to_terminals[connection]:
                                del self.connection_to_terminals[connection]
                    except Exception:
                        pass
        
        # Schedule the color setting to run after the terminal is fully initialized
        GLib.idle_add(_set_terminal_colors)

    def _on_disconnect_confirmed(self, dialog, response_id, connection):
        """Handle response from disconnect confirmation dialog"""
        dialog.destroy()
        if response_id == 'disconnect' and connection in self.active_terminals:
            terminal = self.active_terminals[connection]
            terminal.disconnect()
            # If part of a delete flow, remove the connection now
            if getattr(self, '_pending_delete_connection', None) is connection:
                try:
                    self.connection_manager.remove_connection(connection)
                finally:
                    self._pending_delete_connection = None
    
    def disconnect_from_host(self, connection: Connection):
        """Disconnect from SSH host"""
        if connection not in self.active_terminals:
            return
            
        # Check if confirmation is required
        confirm_disconnect = self.config.get_setting('confirm-disconnect', True)
        
        if confirm_disconnect:
            # Show confirmation dialog
            dialog = Adw.MessageDialog(
                transient_for=self,
                modal=True,
                heading=_("Disconnect from {}").format(connection.nickname or connection.host),
                body=_("Are you sure you want to disconnect from this host?")
            )
            dialog.add_response('cancel', _("Cancel"))
            dialog.add_response('disconnect', _("Disconnect"))
            dialog.set_response_appearance('disconnect', Adw.ResponseAppearance.DESTRUCTIVE)
            dialog.set_default_response('close')
            dialog.set_close_response('cancel')
            
            dialog.connect('response', self._on_disconnect_confirmed, connection)
            dialog.present()
        else:
            # Disconnect immediately without confirmation
            terminal = self.active_terminals[connection]
            terminal.disconnect()

    # Signal handlers
    def on_connection_click(self, gesture, n_press, x, y):
        """Handle clicks on the connection list"""
        # Get the row that was clicked
        row = self.connection_list.get_row_at_y(int(y))
        if row is None:
            return
        
        if n_press == 1:  # Single click - just select
            self.connection_list.select_row(row)
            gesture.set_state(Gtk.EventSequenceState.CLAIMED)
        elif n_press == 2:  # Double click - connect
            self._cycle_connection_tabs_or_open(row.connection)
            gesture.set_state(Gtk.EventSequenceState.CLAIMED)

    def on_connection_activated(self, list_box, row):
        """Handle connection activation (Enter key)"""
        if row:
            self._cycle_connection_tabs_or_open(row.connection)
            

        
    def on_connection_activate(self, list_box, row):
        """Handle connection activation (Enter key or double-click)"""
        if row:
            self._cycle_connection_tabs_or_open(row.connection)
            return True  # Stop event propagation
        return False
        
    def on_activate_connection(self, action, param):
        """Handle the activate-connection action"""
        row = self.connection_list.get_selected_row()
        if row:
            self._cycle_connection_tabs_or_open(row.connection)
            
    def on_connection_activated(self, list_box, row):
        """Handle connection activation (double-click)"""
        if row:
            self._cycle_connection_tabs_or_open(row.connection)

    def _cycle_connection_tabs_or_open(self, connection: Connection):
        """If there are open tabs for this server, cycle to the next one (wrap).
        Otherwise open a new tab for the server.
        """
        try:
            # Collect current pages in visual/tab order
            terms_for_conn = []
            try:
                n = self.tab_view.get_n_pages()
            except Exception:
                n = 0
            for i in range(n):
                page = self.tab_view.get_nth_page(i)
                child = page.get_child() if hasattr(page, 'get_child') else None
                if child is not None and self.terminal_to_connection.get(child) == connection:
                    terms_for_conn.append(child)

            if terms_for_conn:
                # Determine current index among this connection's tabs
                selected = self.tab_view.get_selected_page()
                current_idx = -1
                if selected is not None:
                    current_child = selected.get_child()
                    for i, t in enumerate(terms_for_conn):
                        if t == current_child:
                            current_idx = i
                            break
                # Compute next index (wrap)
                next_idx = (current_idx + 1) % len(terms_for_conn) if current_idx >= 0 else 0
                next_term = terms_for_conn[next_idx]
                page = self.tab_view.get_page(next_term)
                if page is not None:
                    self.tab_view.set_selected_page(page)
                    # Update most-recent mapping
                    self.active_terminals[connection] = next_term
                    return

            # No existing tabs for this connection -> open a new one
            self.connect_to_host(connection, force_new=False)
        except Exception as e:
            logger.error(f"Failed to cycle or open for {getattr(connection, 'nickname', '')}: {e}")

    def on_connection_selected(self, list_box, row):
        """Handle connection list selection change"""
        has_selection = row is not None
        self.edit_button.set_sensitive(has_selection)
        if hasattr(self, 'copy_key_button'):
            self.copy_key_button.set_sensitive(has_selection)
        if hasattr(self, 'upload_button'):
            self.upload_button.set_sensitive(has_selection)
        self.delete_button.set_sensitive(has_selection)

    def on_add_connection_clicked(self, button):
        """Handle add connection button click"""
        self.show_connection_dialog()

    def on_edit_connection_clicked(self, button):
        """Handle edit connection button click"""
        selected_row = self.connection_list.get_selected_row()
        if selected_row:
            self.show_connection_dialog(selected_row.connection)

    def on_copy_key_to_server_clicked(self, button):
        """Copy selected SSH public key to selected server using ssh-copy-id"""
        try:
            selected_row = self.connection_list.get_selected_row()
            if not selected_row:
                return
            connection = getattr(selected_row, 'connection', None)
            if not connection:
                return

            # Discover keys
            keys = self.key_manager.discover_keys() if hasattr(self, 'key_manager') else []
            if not keys:
                # Offer to generate a key first
                dlg = Adw.MessageDialog(
                    transient_for=self,
                    modal=True,
                    heading=_('No SSH keys found'),
                    body=_('You have no SSH keys in ~/.ssh. Generate a new key pair now?')
                )
                dlg.add_response('cancel', _('Cancel'))
                dlg.add_response('generate', _('Generate SSH Key'))
                dlg.set_default_response('generate')
                dlg.set_close_response('cancel')

                def _resp(d, response):
                    d.close()
                    if response == 'generate':
                        self.show_key_dialog()
                dlg.connect('response', _resp)
                dlg.present()
                return

            # Picker dialog for available keys
            picker = Adw.MessageDialog(
                transient_for=self,
                modal=True,
                heading=_('Select SSH key to copy'),
                body=_('Choose which public key to add to the server using ssh-copy-id')
            )
            box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
            names = [os.path.basename(k.path) for k in keys]
            dropdown = Gtk.DropDown.new_from_strings(names)
            dropdown.set_selected(0)
            box.append(dropdown)
            picker.set_extra_child(box)
            picker.add_response('cancel', _('Cancel'))
            picker.add_response('copy', _('Copy Key'))
            picker.set_default_response('copy')
            picker.set_close_response('cancel')

            def _on_pick(d, response):
                d.close()
                if response != 'copy':
                    return
                idx = dropdown.get_selected()
                if idx < 0 or idx >= len(keys):
                    return
                ssh_key = keys[idx]
                if _HAS_VTE:
                    self._show_ssh_copy_id_terminal_using_main_widget(connection, ssh_key)
                else:
                    ok = self.key_manager.copy_key_to_host(ssh_key, connection)
                    if ok:
                        msg = Adw.MessageDialog(
                            transient_for=self,
                            modal=True,
                            heading=_('Success'),
                            body=_('Public key copied to {}@{}').format(connection.username, connection.host)
                        )
                        msg.add_response('ok', _('OK'))
                        msg.set_default_response('ok')
                        msg.set_close_response('ok')
                        msg.present()
                    else:
                        msg = Adw.MessageDialog(
                            transient_for=self,
                            modal=True,
                            heading=_('Error'),
                            body=_('Failed to copy the public key. Check logs for details.')
                        )
                        msg.add_response('ok', _('OK'))
                        msg.set_default_response('ok')
                        msg.set_close_response('ok')
                        msg.present()

            picker.connect('response', _on_pick)
            picker.present()
        except Exception as e:
            logger.error(f'Copy key to server failed: {e}')

    def on_upload_file_clicked(self, button):
        """Show SCP intro dialog and start upload to selected server."""
        try:
            selected_row = self.connection_list.get_selected_row()
            if not selected_row:
                return
            connection = getattr(selected_row, 'connection', None)
            if not connection:
                return

            intro = Adw.MessageDialog(
                transient_for=self,
                modal=True,
                heading=_('Upload files to server'),
                body=_('We will use scp to upload file(s) to the selected server. You will be prompted to choose files and a destination path on the server.')
            )
            intro.add_response('cancel', _('Cancel'))
            intro.add_response('choose', _('Choose files…'))
            intro.set_default_response('choose')
            intro.set_close_response('cancel')

            def _on_intro(dlg, response):
                dlg.close()
                if response != 'choose':
                    return
                # Choose local files
                file_chooser = Gtk.FileChooserDialog(
                    title=_('Select files to upload'),
                    action=Gtk.FileChooserAction.OPEN,
                )
                file_chooser.set_transient_for(self)
                file_chooser.set_modal(True)
                file_chooser.add_button(_('Cancel'), Gtk.ResponseType.CANCEL)
                file_chooser.add_button(_('Open'), Gtk.ResponseType.ACCEPT)
                file_chooser.set_select_multiple(True)
                file_chooser.connect('response', lambda fc, resp: self._on_files_chosen(fc, resp, connection))
                file_chooser.show()

            intro.connect('response', _on_intro)
            intro.present()
        except Exception as e:
            logger.error(f'Upload dialog failed: {e}')

    def _show_ssh_copy_id_terminal_using_main_widget(self, connection, ssh_key):
        """Show a window with header bar and embedded terminal running ssh-copy-id.

        Requirements:
        - Terminal expands horizontally, no borders around it
        - Header bar contains Cancel and Close buttons
        """
        try:
            target = f"{connection.username}@{connection.host}" if getattr(connection, 'username', '') else str(connection.host)
            pub_name = os.path.basename(getattr(ssh_key, 'public_path', '') or '')
            body_text = _('This will add your public key to the server\'s ~/.ssh/authorized_keys so future logins can use SSH keys.')
            dlg = Adw.Window()
            dlg.set_transient_for(self)
            dlg.set_modal(True)
            try:
                dlg.set_title(_('ssh-copy-id'))
            except Exception:
                pass
            try:
                dlg.set_default_size(920, 520)
            except Exception:
                pass

            # Header bar with Cancel
            header = Adw.HeaderBar()
            title_widget = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=2)
            title_label = Gtk.Label(label=_('ssh-copy-id'))
            title_label.set_halign(Gtk.Align.START)
            subtitle_label = Gtk.Label(label=_('Copying {key} to {target}').format(key=pub_name or _('selected key'), target=target))
            subtitle_label.set_halign(Gtk.Align.START)
            try:
                title_label.add_css_class('title-2')
                subtitle_label.add_css_class('dim-label')
            except Exception:
                pass
            title_widget.append(title_label)
            title_widget.append(subtitle_label)
            header.set_title_widget(title_widget)

            cancel_btn = Gtk.Button(label=_('Cancel'))
            try:
                cancel_btn.add_css_class('flat')
            except Exception:
                pass
            header.pack_start(cancel_btn)
            # Close button is omitted; window has native close (X)

            # Content: TerminalWidget without connecting spinner/banner
            content_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
            content_box.set_hexpand(True)
            content_box.set_vexpand(True)
            try:
                content_box.set_margin_top(12)
                content_box.set_margin_bottom(12)
                content_box.set_margin_start(6)
                content_box.set_margin_end(6)
            except Exception:
                pass
            # Optional info text under header bar
            info_lbl = Gtk.Label(label=body_text)
            info_lbl.set_halign(Gtk.Align.START)
            try:
                info_lbl.add_css_class('dim-label')
                info_lbl.set_wrap(True)
            except Exception:
                pass
            content_box.append(info_lbl)

            term_widget = TerminalWidget(connection, self.config, self.connection_manager)
            # Hide connecting overlay and suppress disconnect banner for this non-SSH task
            try:
                term_widget._set_connecting_overlay_visible(False)
                setattr(term_widget, '_suppress_disconnect_banner', True)
                term_widget._set_disconnected_banner_visible(False)
            except Exception:
                pass
            term_widget.set_hexpand(True)
            term_widget.set_vexpand(True)
            # No frame: avoid borders around the terminal
            content_box.append(term_widget)

            # Root container combines header and content
            root_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
            root_box.append(header)
            root_box.append(content_box)
            try:
                dlg.set_content(root_box)
            except Exception:
                # GTK fallback
                dlg.set_child(root_box)

            def _on_cancel(_btn):
                try:
                    if hasattr(term_widget, 'disconnect'):
                        term_widget.disconnect()
                except Exception:
                    pass
                dlg.close()
            cancel_btn.connect('clicked', _on_cancel)
            # No explicit close button; use window close (X)

            # Build ssh-copy-id command with options derived from connection settings
            argv = self._build_ssh_copy_id_argv(connection, ssh_key)
            cmdline = ' '.join([GLib.shell_quote(a) for a in argv])
            logger.info("Starting ssh-copy-id: %s", ' '.join(argv))

            # Helper to write colored lines into the terminal
            def _feed_colored_line(text: str, color: str):
                colors = {
                    'red': '\x1b[31m',
                    'green': '\x1b[32m',
                    'yellow': '\x1b[33m',
                    'blue': '\x1b[34m',
                }
                prefix = colors.get(color, '')
                try:
                    term_widget.vte.feed(("\r\n" + prefix + text + "\x1b[0m\r\n").encode('utf-8'))
                except Exception:
                    pass

            # Initial info line
            _feed_colored_line(_('Running ssh-copy-id…'), 'yellow')

            try:
                term_widget.vte.spawn_async(
                    Vte.PtyFlags.DEFAULT,
                    os.path.expanduser('~') or '/',
                    ['bash', '-lc', cmdline],
                    [f"{k}={v}" for k, v in os.environ.items()],
                    GLib.SpawnFlags.DEFAULT,
                    None,
                    None,
                    -1,
                    None,
                    None
                )

                # Show result modal when the command finishes
                def _on_copyid_exited(_vte, status):
                    # Normalize exit code
                    exit_code = None
                    try:
                        if os.WIFEXITED(status):
                            exit_code = os.WEXITSTATUS(status)
                        else:
                            exit_code = status if 0 <= int(status) < 256 else ((int(status) >> 8) & 0xFF)
                    except Exception:
                        try:
                            exit_code = int(status)
                        except Exception:
                            exit_code = status

                    ok = (exit_code == 0)
                    if ok:
                        _feed_colored_line(_('Public key was installed successfully.'), 'green')
                    else:
                        _feed_colored_line(_('Failed to install the public key.'), 'red')

                    def _present_result_dialog():
                        msg = Adw.MessageDialog(
                            transient_for=dlg,
                            modal=True,
                            heading=_('Success') if ok else _('Error'),
                            body=(_('Public key copied to {}@{}').format(connection.username, connection.host)
                                  if ok else _('Failed to copy the public key. Check logs for details.')),
                        )
                        msg.add_response('ok', _('OK'))
                        msg.set_default_response('ok')
                        msg.set_close_response('ok')
                        msg.present()
                        return False

                    GLib.idle_add(_present_result_dialog)

                try:
                    term_widget.vte.connect('child-exited', _on_copyid_exited)
                except Exception:
                    pass
            except Exception as e:
                logger.error(f'Failed to spawn ssh-copy-id in TerminalWidget: {e}')
                dlg.close()
                ok = self.key_manager.copy_key_to_host(ssh_key, connection)
                msg = Adw.MessageDialog(
                    transient_for=self,
                    modal=True,
                    heading=_('Success') if ok else _('Error'),
                    body=(_('Public key copied to {}@{}').format(connection.username, connection.host)
                         if ok else _('Failed to copy the public key. Check logs for details.'))
                )
                msg.add_response('ok', _('OK'))
                msg.set_default_response('ok')
                msg.set_close_response('ok')
                msg.present()
                return

            dlg.present()
        except Exception as e:
            logger.error(f'VTE ssh-copy-id window failed: {e}')

    def _build_ssh_copy_id_argv(self, connection, ssh_key):
        """Construct argv for ssh-copy-id honoring saved UI auth preferences."""
        argv = ['ssh-copy-id', '-i', ssh_key.public_path]
        try:
            if getattr(connection, 'port', 22) and connection.port != 22:
                argv += ['-p', str(connection.port)]
        except Exception:
            pass
        # Honor app SSH settings: strict host key checking / auto-add
        try:
            cfg = Config()
            ssh_cfg = cfg.get_ssh_config() if hasattr(cfg, 'get_ssh_config') else {}
            strict_val = str(ssh_cfg.get('strict_host_key_checking', '') or '').strip()
            auto_add = bool(ssh_cfg.get('auto_add_host_keys', True))
            if strict_val:
                argv += ['-o', f'StrictHostKeyChecking={strict_val}']
            elif auto_add:
                argv += ['-o', 'StrictHostKeyChecking=accept-new']
        except Exception:
            argv += ['-o', 'StrictHostKeyChecking=accept-new']
        # Derive auth prefs from saved config and connection
        prefer_password = False
        key_mode = 0
        keyfile = getattr(connection, 'keyfile', '') or ''
        try:
            cfg = Config()
            meta = cfg.get_connection_meta(connection.nickname) if hasattr(cfg, 'get_connection_meta') else {}
            if isinstance(meta, dict) and 'auth_method' in meta:
                prefer_password = int(meta.get('auth_method', 0) or 0) == 1
        except Exception:
            try:
                prefer_password = int(getattr(connection, 'auth_method', 0) or 0) == 1
            except Exception:
                prefer_password = False
        try:
            # key_select_mode is saved in ssh config, our connection object should have it post-load
            key_mode = int(getattr(connection, 'key_select_mode', 0) or 0)
        except Exception:
            key_mode = 0
        # Validate keyfile path
        try:
            keyfile_ok = bool(keyfile) and os.path.isfile(keyfile)
        except Exception:
            keyfile_ok = False

        # Priority: if UI selected a specific key and it exists, use it; otherwise fall back to password prefs/try-all
        if key_mode == 1 and keyfile_ok:
            argv += ['-o', f'IdentityFile={keyfile}', '-o', 'IdentitiesOnly=yes', '-o', 'IdentityAgent=none']
        else:
            # Only force password when user selected password auth
            if prefer_password:
                argv += ['-o', 'PubkeyAuthentication=no', '-o', 'PreferredAuthentications=password,keyboard-interactive']
        # Target
        target = f"{connection.username}@{connection.host}" if getattr(connection, 'username', '') else str(connection.host)
        argv.append(target)
        return argv

    def _on_files_chosen(self, chooser, response, connection):
        try:
            if response != Gtk.ResponseType.ACCEPT:
                chooser.destroy()
                return
            files = chooser.get_files()
            chooser.destroy()
            if not files:
                return
            # Ask remote destination path
            prompt = Adw.MessageDialog(
                transient_for=self,
                modal=True,
                heading=_('Remote destination'),
                body=_('Enter a remote directory (e.g., ~/ or /var/tmp). Files will be uploaded using scp.')
            )
            box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
            dest_row = Adw.EntryRow(title=_('Remote directory'))
            dest_row.set_text('~')
            box.append(dest_row)
            prompt.set_extra_child(box)
            prompt.add_response('cancel', _('Cancel'))
            prompt.add_response('upload', _('Upload'))
            prompt.set_default_response('upload')
            prompt.set_close_response('cancel')

            def _go(d, resp):
                d.close()
                if resp != 'upload':
                    return
                remote_dir = dest_row.get_text().strip() or '~'
                self._start_scp_upload(connection, [f.get_path() for f in files], remote_dir)

            prompt.connect('response', _go)
            prompt.present()
        except Exception as e:
            logger.error(f'File selection failed: {e}')

    def _start_scp_upload(self, connection, local_paths, remote_dir):
        """Run scp using the same terminal window layout as ssh-copy-id."""
        try:
            self._show_scp_upload_terminal_window(connection, local_paths, remote_dir)
        except Exception as e:
            logger.error(f'scp upload failed to start: {e}')

    def _show_scp_upload_terminal_window(self, connection, local_paths, remote_dir):
        try:
            target = f"{connection.username}@{connection.host}"
            info_text = _('We will use scp to upload file(s) to the selected server.')

            dlg = Adw.Window()
            dlg.set_transient_for(self)
            dlg.set_modal(True)
            try:
                dlg.set_title(_('Upload files (scp)'))
            except Exception:
                pass
            try:
                dlg.set_default_size(920, 520)
            except Exception:
                pass

            # Header bar with Cancel
            header = Adw.HeaderBar()
            title_widget = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=2)
            title_label = Gtk.Label(label=_('Upload files (scp)'))
            title_label.set_halign(Gtk.Align.START)
            subtitle_label = Gtk.Label(label=_('Uploading to {target}:{dir}').format(target=target, dir=remote_dir))
            subtitle_label.set_halign(Gtk.Align.START)
            try:
                title_label.add_css_class('title-2')
                subtitle_label.add_css_class('dim-label')
            except Exception:
                pass
            title_widget.append(title_label)
            title_widget.append(subtitle_label)
            header.set_title_widget(title_widget)

            cancel_btn = Gtk.Button(label=_('Cancel'))
            try:
                cancel_btn.add_css_class('flat')
            except Exception:
                pass
            header.pack_start(cancel_btn)

            # Content area
            content_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
            content_box.set_hexpand(True)
            content_box.set_vexpand(True)
            try:
                content_box.set_margin_top(12)
                content_box.set_margin_bottom(12)
                content_box.set_margin_start(6)
                content_box.set_margin_end(6)
            except Exception:
                pass

            info_lbl = Gtk.Label(label=info_text)
            info_lbl.set_halign(Gtk.Align.START)
            try:
                info_lbl.add_css_class('dim-label')
                info_lbl.set_wrap(True)
            except Exception:
                pass
            content_box.append(info_lbl)

            term_widget = TerminalWidget(connection, self.config, self.connection_manager)
            try:
                term_widget._set_connecting_overlay_visible(False)
                setattr(term_widget, '_suppress_disconnect_banner', True)
                term_widget._set_disconnected_banner_visible(False)
            except Exception:
                pass
            term_widget.set_hexpand(True)
            term_widget.set_vexpand(True)
            content_box.append(term_widget)

            root_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
            root_box.append(header)
            root_box.append(content_box)
            try:
                dlg.set_content(root_box)
            except Exception:
                dlg.set_child(root_box)

            def _on_cancel(_btn):
                try:
                    if hasattr(term_widget, 'disconnect'):
                        term_widget.disconnect()
                except Exception:
                    pass
                dlg.close()
            cancel_btn.connect('clicked', _on_cancel)

            # Build and run scp command in the terminal
            argv = self._build_scp_argv(connection, local_paths, remote_dir)
            cmdline = ' '.join([GLib.shell_quote(a) for a in argv])

            # Helper to write colored lines
            def _feed_colored_line(text: str, color: str):
                colors = {
                    'red': '\x1b[31m',
                    'green': '\x1b[32m',
                    'yellow': '\x1b[33m',
                    'blue': '\x1b[34m',
                }
                prefix = colors.get(color, '')
                try:
                    term_widget.vte.feed(("\r\n" + prefix + text + "\x1b[0m\r\n").encode('utf-8'))
                except Exception:
                    pass

            _feed_colored_line(_('Starting upload…'), 'yellow')

            try:
                term_widget.vte.spawn_async(
                    Vte.PtyFlags.DEFAULT,
                    os.path.expanduser('~') or '/',
                    ['bash', '-lc', cmdline],
                    [f"{k}={v}" for k, v in os.environ.items()],
                    GLib.SpawnFlags.DEFAULT,
                    None,
                    None,
                    -1,
                    None,
                    None
                )

                def _on_scp_exited(_vte, status):
                    # Normalize exit code
                    exit_code = None
                    try:
                        if os.WIFEXITED(status):
                            exit_code = os.WEXITSTATUS(status)
                        else:
                            exit_code = status if 0 <= int(status) < 256 else ((int(status) >> 8) & 0xFF)
                    except Exception:
                        try:
                            exit_code = int(status)
                        except Exception:
                            exit_code = status
                    ok = (exit_code == 0)
                    if ok:
                        _feed_colored_line(_('Upload finished successfully.'), 'green')
                    else:
                        _feed_colored_line(_('Upload failed. See output above.'), 'red')

                    def _present_result_dialog():
                        msg = Adw.MessageDialog(
                            transient_for=dlg,
                            modal=True,
                            heading=_('Upload complete') if ok else _('Upload failed'),
                            body=(_('Files uploaded to {target}:{dir}').format(target=target, dir=remote_dir)
                                  if ok else _('scp exited with an error. Please review the log output.')),
                        )
                        msg.add_response('ok', _('OK'))
                        msg.set_default_response('ok')
                        msg.set_close_response('ok')
                        msg.present()
                        return False

                    GLib.idle_add(_present_result_dialog)

                try:
                    term_widget.vte.connect('child-exited', _on_scp_exited)
                except Exception:
                    pass
            except Exception as e:
                logger.error(f'Failed to spawn scp in TerminalWidget: {e}')
                dlg.close()
                # Fallback could be implemented here if needed
                return

            dlg.present()
        except Exception as e:
            logger.error(f'Failed to open scp terminal window: {e}')

    def _build_scp_argv(self, connection, local_paths, remote_dir):
        argv = ['scp', '-v']
        # Port
        try:
            if getattr(connection, 'port', 22) and connection.port != 22:
                argv += ['-P', str(connection.port)]
        except Exception:
            pass
        # Auth/SSH options similar to ssh-copy-id
        try:
            cfg = Config()
            ssh_cfg = cfg.get_ssh_config() if hasattr(cfg, 'get_ssh_config') else {}
            strict_val = str(ssh_cfg.get('strict_host_key_checking', '') or '').strip()
            auto_add = bool(ssh_cfg.get('auto_add_host_keys', True))
            if strict_val:
                argv += ['-o', f'StrictHostKeyChecking={strict_val}']
            elif auto_add:
                argv += ['-o', 'StrictHostKeyChecking=accept-new']
        except Exception:
            argv += ['-o', 'StrictHostKeyChecking=accept-new']
        # Prefer password if selected
        prefer_password = False
        key_mode = 0
        keyfile = getattr(connection, 'keyfile', '') or ''
        try:
            cfg = Config()
            meta = cfg.get_connection_meta(connection.nickname) if hasattr(cfg, 'get_connection_meta') else {}
            if isinstance(meta, dict) and 'auth_method' in meta:
                prefer_password = int(meta.get('auth_method', 0) or 0) == 1
        except Exception:
            try:
                prefer_password = int(getattr(connection, 'auth_method', 0) or 0) == 1
            except Exception:
                prefer_password = False
        try:
            key_mode = int(getattr(connection, 'key_select_mode', 0) or 0)
        except Exception:
            key_mode = 0
        try:
            keyfile_ok = bool(keyfile) and os.path.isfile(keyfile)
        except Exception:
            keyfile_ok = False
        if key_mode == 1 and keyfile_ok:
            argv += ['-i', keyfile, '-o', 'IdentitiesOnly=yes', '-o', 'IdentityAgent=none']
        elif prefer_password:
            argv += ['-o', 'PubkeyAuthentication=no', '-o', 'PreferredAuthentications=password,keyboard-interactive']
        # Paths
        for p in local_paths:
            argv.append(p)
        target = f"{connection.username}@{connection.host}" if getattr(connection, 'username', '') else str(connection.host)
        argv.append(f"{target}:{remote_dir}")
        return argv

    def on_delete_connection_clicked(self, button):
        """Handle delete connection button click"""
        selected_row = self.connection_list.get_selected_row()
        if not selected_row:
            return
        
        connection = selected_row.connection
        
        # If host has active connections/tabs, warn about closing them first
        has_active_terms = bool(self.connection_to_terminals.get(connection, []))
        if getattr(connection, 'is_connected', False) or has_active_terms:
            dialog = Adw.MessageDialog(
                transient_for=self,
                modal=True,
                heading=_('Remove host?'),
                body=_('Close connections and remove host?')
            )
            dialog.add_response('cancel', _('Cancel'))
            dialog.add_response('close_remove', _('Close and Remove'))
            dialog.set_response_appearance('close_remove', Adw.ResponseAppearance.DESTRUCTIVE)
            dialog.set_default_response('close')
            dialog.set_close_response('cancel')
        else:
            # Simple delete confirmation when not connected
            dialog = Adw.MessageDialog.new(self, _('Delete Connection?'),
                                         _('Are you sure you want to delete "{}"?').format(connection.nickname))
            dialog.add_response('cancel', _('Cancel'))
            dialog.add_response('delete', _('Delete'))
            dialog.set_response_appearance('delete', Adw.ResponseAppearance.DESTRUCTIVE)
            dialog.set_default_response('cancel')
            dialog.set_close_response('cancel')

        dialog.connect('response', self.on_delete_connection_response, connection)
        dialog.present()

    def on_delete_connection_response(self, dialog, response, connection):
        """Handle delete connection dialog response"""
        if response == 'delete':
            # Simple deletion when not connected
            self.connection_manager.remove_connection(connection)
        elif response == 'close_remove':
            # Close connections immediately (no extra confirmation), then remove
            try:
                # Disconnect all terminals for this connection
                for term in list(self.connection_to_terminals.get(connection, [])):
                    try:
                        if hasattr(term, 'disconnect'):
                            term.disconnect()
                    except Exception:
                        pass
                # Also disconnect the active terminal if tracked separately
                term = self.active_terminals.get(connection)
                if term and hasattr(term, 'disconnect'):
                    try:
                        term.disconnect()
                    except Exception:
                        pass
            finally:
                # Remove connection without further prompts
                self.connection_manager.remove_connection(connection)

    def _on_tab_close_confirmed(self, dialog, response_id, tab_view, page):
        """Handle response from tab close confirmation dialog"""
        dialog.destroy()
        if response_id == 'close':
            self._close_tab(tab_view, page)
        # If cancelled, do nothing - the tab remains open
    
    def _close_tab(self, tab_view, page):
        """Close the tab and clean up resources"""
        if hasattr(page, 'get_child'):
            child = page.get_child()
            if hasattr(child, 'disconnect'):
                # Get the connection associated with this terminal using reverse map
                connection = self.terminal_to_connection.get(child)
                # Disconnect the terminal
                child.disconnect()
                # Clean up multi-tab tracking maps
                try:
                    if connection is not None:
                        # Remove from list for this connection
                        if connection in self.connection_to_terminals and child in self.connection_to_terminals[connection]:
                            self.connection_to_terminals[connection].remove(child)
                            if not self.connection_to_terminals[connection]:
                                del self.connection_to_terminals[connection]
                        # Update most-recent mapping
                        if connection in self.active_terminals and self.active_terminals[connection] is child:
                            remaining = self.connection_to_terminals.get(connection)
                            if remaining:
                                self.active_terminals[connection] = remaining[-1]
                            else:
                                del self.active_terminals[connection]
                    if child in self.terminal_to_connection:
                        del self.terminal_to_connection[child]
                except Exception:
                    pass
        
        # Close the tab page
        tab_view.close_page(page)
        
        # Update the UI based on the number of remaining tabs
        GLib.idle_add(self._update_ui_after_tab_close)
    
    def on_tab_close(self, tab_view, page):
        """Handle tab close - THE KEY FIX: Never call close_page ourselves"""
        # If we are closing pages programmatically (e.g., after deleting a
        # connection), suppress the confirmation dialog and allow the default
        # close behavior to proceed.
        if getattr(self, '_suppress_close_confirmation', False):
            return False
        # Get the connection for this tab
        connection = None
        terminal = None
        if hasattr(page, 'get_child'):
            child = page.get_child()
            if hasattr(child, 'disconnect'):
                terminal = child
                connection = self.terminal_to_connection.get(child)
        
        if not connection:
            # For non-terminal tabs, allow immediate close
            return False  # Allow the default close behavior
        
        # Check if confirmation is required
        confirm_disconnect = self.config.get_setting('confirm-disconnect', True)
        
        if confirm_disconnect:
            # Store tab view and page as instance variables
            self._pending_close_tab_view = tab_view
            self._pending_close_page = page
            self._pending_close_connection = connection
            self._pending_close_terminal = terminal
            
            # Show confirmation dialog
            dialog = Adw.MessageDialog(
                transient_for=self,
                modal=True,
                heading=_("Close connection to {}").format(connection.nickname or connection.host),
                body=_("Are you sure you want to close this connection?")
            )
            dialog.add_response('cancel', _("Cancel"))
            dialog.add_response('close', _("Close"))
            dialog.set_response_appearance('close', Adw.ResponseAppearance.DESTRUCTIVE)
            dialog.set_default_response('close')
            dialog.set_close_response('cancel')
            
            # Connect to response signal before showing the dialog
            dialog.connect('response', self._on_tab_close_response)
            dialog.present()
            
            # Prevent the default close behavior while we show confirmation
            return True
        else:
            # If no confirmation is needed, just allow the default close behavior.
            # The default handler will close the page, which in turn triggers the
            # terminal disconnection via the page's 'unmap' or 'destroy' signal.
            return False

    def _on_tab_close_response(self, dialog, response_id):
        """Handle the response from the close confirmation dialog."""
        # Retrieve the pending tab info
        tab_view = self._pending_close_tab_view
        page = self._pending_close_page
        terminal = self._pending_close_terminal

        if response_id == 'close':
            # User confirmed, disconnect the terminal. The tab will be removed
            # by the AdwTabView once we finish the close operation.
            if terminal and hasattr(terminal, 'disconnect'):
                terminal.disconnect()
            # Now, tell the tab view to finish closing the page.
            tab_view.close_page_finish(page, True)
            
            # Check if this was the last tab and show welcome screen if needed
            if tab_view.get_n_pages() == 0:
                self.show_welcome_view()
        else:
            # User cancelled, so we reject the close request.
            # This is the critical step that makes the close button work again.
            tab_view.close_page_finish(page, False)

        dialog.destroy()
        # Clear pending state to avoid memory leaks
        self._pending_close_tab_view = None
        self._pending_close_page = None
        self._pending_close_connection = None
        self._pending_close_terminal = None
    
    def on_tab_attached(self, tab_view, page, position):
        """Handle tab attached"""
        pass

    def on_tab_detached(self, tab_view, page, position):
        """Handle tab detached"""
        # Cleanup terminal-to-connection maps when a page is detached
        try:
            if hasattr(page, 'get_child'):
                child = page.get_child()
                if child in self.terminal_to_connection:
                    connection = self.terminal_to_connection.get(child)
                    # Remove reverse map
                    del self.terminal_to_connection[child]
                    # Remove from per-connection list
                    if connection in self.connection_to_terminals and child in self.connection_to_terminals[connection]:
                        self.connection_to_terminals[connection].remove(child)
                        if not self.connection_to_terminals[connection]:
                            del self.connection_to_terminals[connection]
                    # Update most recent mapping if needed
                    if connection in self.active_terminals and self.active_terminals[connection] is child:
                        remaining = self.connection_to_terminals.get(connection)
                        if remaining:
                            self.active_terminals[connection] = remaining[-1]
                        else:
                            del self.active_terminals[connection]
        except Exception:
            pass

        # Show welcome view if no more tabs are left
        if tab_view.get_n_pages() == 0:
            self.show_welcome_view()

    def on_terminal_connected(self, terminal):
        """Handle terminal connection established"""
        # Update the connection's is_connected status
        terminal.connection.is_connected = True
        
        # Update connection row status
        if terminal.connection in self.connection_rows:
            row = self.connection_rows[terminal.connection]
            row.update_status()
            row.queue_draw()  # Force redraw
        
        # Hide reconnecting feedback if visible and reset controlled flag
        GLib.idle_add(self._hide_reconnecting_message)
        self._is_controlled_reconnect = False

        # Log connection event
        if not getattr(self, '_is_controlled_reconnect', False):
            logger.info(f"Terminal connected: {terminal.connection.nickname} ({terminal.connection.username}@{terminal.connection.host})")
        else:
            logger.debug(f"Terminal reconnected after settings update: {terminal.connection.nickname}")

    def on_terminal_disconnected(self, terminal):
        """Handle terminal connection lost"""
        # Update the connection's is_connected status
        terminal.connection.is_connected = False
        
        # Update connection row status
        if terminal.connection in self.connection_rows:
            row = self.connection_rows[terminal.connection]
            row.update_status()
            row.queue_draw()  # Force redraw
            
        logger.info(f"Terminal disconnected: {terminal.connection.nickname} ({terminal.connection.username}@{terminal.connection.host})")
        
        # Do not reset controlled reconnect flag here; it is managed by the
        # reconnection flow (_on_reconnect_response/_reset_controlled_reconnect)

        # Toasts are disabled per user preference; no notification here.
        pass
            
    def on_connection_added(self, manager, connection):
        """Handle new connection added to the connection manager"""
        logger.info(f"New connection added: {connection.nickname}")
        self.add_connection_row(connection)
        
    def on_terminal_title_changed(self, terminal, title):
        """Handle terminal title change"""
        # Update the tab title with the new terminal title
        page = self.tab_view.get_page(terminal)
        if page:
            if title and title != terminal.connection.nickname:
                page.set_title(f"{terminal.connection.nickname} - {title}")
            else:
                page.set_title(terminal.connection.nickname)
                
    def on_connection_removed(self, manager, connection):
        """Handle connection removed from the connection manager"""
        logger.info(f"Connection removed: {connection.nickname}")

        # Remove from UI if it exists
        if connection in self.connection_rows:
            row = self.connection_rows[connection]
            self.connection_list.remove(row)
            del self.connection_rows[connection]

        # Close all terminals for this connection and clean up maps
        terminals = list(self.connection_to_terminals.get(connection, []))
        # Suppress confirmation while we programmatically close pages
        self._suppress_close_confirmation = True
        try:
            for term in terminals:
                try:
                    page = self.tab_view.get_page(term)
                    if page:
                        self.tab_view.close_page(page)
                except Exception:
                    pass
                try:
                    if hasattr(term, 'disconnect'):
                        term.disconnect()
                except Exception:
                    pass
                # Remove reverse map entry for each terminal
                try:
                    if term in self.terminal_to_connection:
                        del self.terminal_to_connection[term]
                except Exception:
                    pass
        finally:
            self._suppress_close_confirmation = False
        if connection in self.connection_to_terminals:
            del self.connection_to_terminals[connection]
        if connection in self.active_terminals:
            del self.active_terminals[connection]



    def on_connection_added(self, manager, connection):
        """Handle new connection added"""
        self.add_connection_row(connection)

    def on_connection_removed(self, manager, connection):
        """Handle connection removed (multi-tab aware)"""
        # Remove from UI
        if connection in self.connection_rows:
            row = self.connection_rows[connection]
            self.connection_list.remove(row)
            del self.connection_rows[connection]

        # Close all terminals for this connection and clean up maps
        terminals = list(self.connection_to_terminals.get(connection, []))
        # Suppress confirmation while we programmatically close pages
        self._suppress_close_confirmation = True
        try:
            for term in terminals:
                try:
                    page = self.tab_view.get_page(term)
                    if page:
                        self.tab_view.close_page(page)
                except Exception:
                    pass
                try:
                    if hasattr(term, 'disconnect'):
                        term.disconnect()
                except Exception:
                    pass
                # Remove reverse map entry for each terminal
                try:
                    if term in self.terminal_to_connection:
                        del self.terminal_to_connection[term]
                except Exception:
                    pass
        finally:
            self._suppress_close_confirmation = False
        if connection in self.connection_to_terminals:
            del self.connection_to_terminals[connection]
        if connection in self.active_terminals:
            del self.active_terminals[connection]

    def on_connection_status_changed(self, manager, connection, is_connected):
        """Handle connection status change"""
        logger.debug(f"Connection status changed: {connection.nickname} - {'Connected' if is_connected else 'Disconnected'}")
        if connection in self.connection_rows:
            row = self.connection_rows[connection]
            # Force update the connection's is_connected state
            connection.is_connected = is_connected
            # Update the row's status
            row.update_status()
            # Force a redraw of the row
            row.queue_draw()

        # If this was a controlled reconnect and we are now connected, hide feedback
        if is_connected and getattr(self, '_is_controlled_reconnect', False):
            GLib.idle_add(self._hide_reconnecting_message)
            self._is_controlled_reconnect = False

        # Use the same reliable status to control terminal banners
        try:
            for term in self.connection_to_terminals.get(connection, []) or []:
                if hasattr(term, '_set_disconnected_banner_visible'):
                    if is_connected:
                        term._set_disconnected_banner_visible(False)
                    else:
                        # Do not force-show here to avoid duplicate messages; terminals handle showing on failure/loss
                        pass
        except Exception:
            pass

    def on_setting_changed(self, config, key, value):
        """Handle configuration setting change"""
        logger.debug(f"Setting changed: {key} = {value}")
        
        # Apply relevant changes
        if key.startswith('terminal.'):
            # Update terminal themes/fonts
            for terms in self.connection_to_terminals.values():
                for terminal in terms:
                    terminal.apply_theme()

    def on_window_size_changed(self, window, param):
        """Handle window size change"""
        width = self.get_default_size()[0]
        height = self.get_default_size()[1]
        sidebar_width = self._get_sidebar_width()
        
        self.config.save_window_geometry(width, height, sidebar_width)

    def simple_close_handler(self, window):
        """Handle window close - distinguish between tab close and window close"""
        logger.info("")
        
        try:
            # Check if we have any tabs open
            n_pages = self.tab_view.get_n_pages()
            logger.info(f" Number of tabs: {n_pages}")
            
            # If we have tabs, close all tabs first and then quit
            if n_pages > 0:
                logger.info(" CLOSING ALL TABS FIRST")
                # Close all tabs
                while self.tab_view.get_n_pages() > 0:
                    page = self.tab_view.get_nth_page(0)
                    self.tab_view.close_page(page)
            
            # Now quit the application
            logger.info(" QUITTING APPLICATION")
            app = self.get_application()
            if app:
                app.quit()
                
        except Exception as e:
            logger.error(f" ERROR IN WINDOW CLOSE: {e}")
            # Force quit even if there's an error
            app = self.get_application()
            self.show_quit_confirmation_dialog()
            return False  # Don't quit yet, let dialog handle it
        
        # No active connections, safe to quit
        self._do_quit()
        return True  # Safe to quit

    def on_close_request(self, window):
        """Handle window close request - MAIN ENTRY POINT"""
        if self._is_quitting:
            return False  # Already quitting, allow close
            
        # Check for active connections across all tabs
        actually_connected = {}
        for conn, terms in self.connection_to_terminals.items():
            for term in terms:
                if getattr(term, 'is_connected', False):
                    actually_connected.setdefault(conn, []).append(term)
        if actually_connected:
            self.show_quit_confirmation_dialog()
            return True  # Prevent close, let dialog handle it
        
        # No active connections, safe to close
        return False  # Allow close

    def show_quit_confirmation_dialog(self):
        """Show confirmation dialog when quitting with active connections"""
        # Only count terminals that are actually connected across all tabs
        connected_items = []
        for conn, terms in self.connection_to_terminals.items():
            for term in terms:
                if getattr(term, 'is_connected', False):
                    connected_items.append((conn, term))
        active_count = len(connected_items)
        connection_names = [conn.nickname for conn, _ in connected_items]
        
        if active_count == 1:
            message = f"You have 1 active SSH connection to '{connection_names[0]}'."
            detail = "Closing the application will disconnect this connection."
        else:
            message = f"You have {active_count} active SSH connections."
            detail = f"Closing the application will disconnect all connections:\n• " + "\n• ".join(connection_names)
        
        dialog = Adw.AlertDialog()
        dialog.set_heading("Active SSH Connections")
        dialog.set_body(f"{message}\n\n{detail}")
        
        dialog.add_response('cancel', 'Cancel')
        dialog.add_response('quit', 'Quit Anyway')
        dialog.set_response_appearance('quit', Adw.ResponseAppearance.DESTRUCTIVE)
        dialog.set_default_response('cancel')
        dialog.set_close_response('cancel')
        
        dialog.connect('response', self.on_quit_confirmation_response)
        dialog.present(self)
    
    def on_quit_confirmation_response(self, dialog, response):
        """Handle quit confirmation dialog response"""
        dialog.close()
        
        if response == 'quit':
            # Start cleanup process
            self._cleanup_and_quit()

    def on_open_new_connection_action(self, action, param=None):
        """Open a new tab for the selected connection via context menu."""
        try:
            connection = getattr(self, '_context_menu_connection', None)
            if connection is None:
                # Fallback to selected row if any
                row = self.connection_list.get_selected_row()
                connection = getattr(row, 'connection', None) if row else None
            if connection is None:
                return
            self.connect_to_host(connection, force_new=True)
        except Exception as e:
            logger.error(f"Failed to open new connection tab: {e}")

    def _cleanup_and_quit(self):
        """Clean up all connections and quit - SIMPLIFIED VERSION"""
        if self._is_quitting:
            logger.debug("Already quitting, ignoring duplicate request")
            return
                
        logger.info("Starting cleanup before quit...")
        self._is_quitting = True
        
        # Get list of all terminals to disconnect
        connections_to_disconnect = []
        for conn, terms in self.connection_to_terminals.items():
            for term in terms:
                connections_to_disconnect.append((conn, term))
        
        if not connections_to_disconnect:
            # No connections to clean up, quit immediately
            self._do_quit()
            return
        
        # Show progress dialog and perform cleanup on idle so the dialog is visible immediately
        total = len(connections_to_disconnect)
        self._show_cleanup_progress(total)
        # Schedule cleanup to run after the dialog has a chance to render
        GLib.idle_add(self._perform_cleanup_and_quit, connections_to_disconnect, priority=GLib.PRIORITY_DEFAULT_IDLE)

    def _perform_cleanup_and_quit(self, connections_to_disconnect):
        """Disconnect terminals with UI progress, then quit. Runs on idle."""
        try:
            total = len(connections_to_disconnect)
            for index, (connection, terminal) in enumerate(connections_to_disconnect, start=1):
                try:
                    logger.debug(f"Disconnecting {connection.nickname} ({index}/{total})")
                    # Always try to cancel any pending SSH spawn quickly first
                    if hasattr(terminal, 'process_pid') and terminal.process_pid:
                        try:
                            import os, signal
                            os.kill(terminal.process_pid, signal.SIGTERM)
                        except Exception:
                            pass
                    # Skip normal disconnect if terminal not connected to avoid hangs
                    if hasattr(terminal, 'is_connected') and not terminal.is_connected:
                        logger.debug("Terminal not connected; skipped disconnect")
                    else:
                        self._disconnect_terminal_safely(terminal)
                finally:
                    # Update progress even if a disconnect fails
                    self._update_cleanup_progress(index, total)
                    # Yield to main loop to keep UI responsive
                    GLib.MainContext.default().iteration(False)
        except Exception as e:
            logger.error(f"Cleanup during quit encountered an error: {e}")
        finally:
            # Clear active terminals and hide progress
            self.active_terminals.clear()
            self._hide_cleanup_progress()
            # Quit on next idle to flush UI updates
            GLib.idle_add(self._do_quit)
        return False  # Do not repeat

    def _show_cleanup_progress(self, total_connections):
        """Show cleanup progress dialog"""
        self._progress_dialog = Gtk.Window()
        self._progress_dialog.set_title("Closing Connections")
        self._progress_dialog.set_transient_for(self)
        self._progress_dialog.set_modal(True)
        self._progress_dialog.set_default_size(350, 120)
        self._progress_dialog.set_resizable(False)
        
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        box.set_margin_top(20)
        box.set_margin_bottom(20)
        box.set_margin_start(20)
        box.set_margin_end(20)
        
        # Progress bar
        self._progress_bar = Gtk.ProgressBar()
        self._progress_bar.set_fraction(0)
        box.append(self._progress_bar)
        
        # Status label
        self._progress_label = Gtk.Label()
        self._progress_label.set_text(f"Closing {total_connections} connection(s)...")
        box.append(self._progress_label)
        
        self._progress_dialog.set_child(box)
        self._progress_dialog.present()

    def _update_cleanup_progress(self, completed, total):
        """Update cleanup progress"""
        if hasattr(self, '_progress_bar') and self._progress_bar:
            fraction = completed / total if total > 0 else 1.0
            self._progress_bar.set_fraction(fraction)
            
        if hasattr(self, '_progress_label') and self._progress_label:
            self._progress_label.set_text(f"Closed {completed} of {total} connection(s)...")

    def _hide_cleanup_progress(self):
        """Hide cleanup progress dialog"""
        if hasattr(self, '_progress_dialog') and self._progress_dialog:
            try:
                self._progress_dialog.close()
                self._progress_dialog = None
                self._progress_bar = None
                self._progress_label = None
            except Exception as e:
                logger.debug(f"Error closing progress dialog: {e}")

    def _show_reconnecting_message(self, connection):
        """Show a small modal indicating reconnection is in progress"""
        try:
            # Avoid duplicate dialogs
            if hasattr(self, '_reconnect_dialog') and self._reconnect_dialog:
                return

            self._reconnect_dialog = Gtk.Window()
            self._reconnect_dialog.set_title(_("Reconnecting"))
            self._reconnect_dialog.set_transient_for(self)
            self._reconnect_dialog.set_modal(True)
            self._reconnect_dialog.set_default_size(320, 100)
            self._reconnect_dialog.set_resizable(False)

            box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
            box.set_margin_top(16)
            box.set_margin_bottom(16)
            box.set_margin_start(16)
            box.set_margin_end(16)

            spinner = Gtk.Spinner()
            spinner.set_hexpand(False)
            spinner.set_vexpand(False)
            spinner.start()
            box.append(spinner)

            label = Gtk.Label()
            label.set_text(_("Reconnecting to {}...").format(getattr(connection, "nickname", "")))
            label.set_halign(Gtk.Align.START)
            label.set_hexpand(True)
            box.append(label)

            self._reconnect_spinner = spinner
            self._reconnect_label = label
            self._reconnect_dialog.set_child(box)
            self._reconnect_dialog.present()
        except Exception as e:
            logger.debug(f"Failed to show reconnecting message: {e}")

    def _hide_reconnecting_message(self):
        """Hide the reconnection progress dialog if shown"""
        try:
            if hasattr(self, '_reconnect_dialog') and self._reconnect_dialog:
                self._reconnect_dialog.close()
            self._reconnect_dialog = None
            self._reconnect_spinner = None
            self._reconnect_label = None
        except Exception as e:
            logger.debug(f"Failed to hide reconnecting message: {e}")

    def _disconnect_terminal_safely(self, terminal):
        """Safely disconnect a terminal"""
        try:
            # Try multiple disconnect methods in order of preference
            if hasattr(terminal, 'disconnect'):
                terminal.disconnect()
            elif hasattr(terminal, 'close_connection'):
                terminal.close_connection()
            elif hasattr(terminal, 'close'):
                terminal.close()
                
            # Force any remaining processes to close
            if hasattr(terminal, 'force_close'):
                terminal.force_close()
                
        except Exception as e:
            logger.error(f"Error disconnecting terminal: {e}")

    def _do_quit(self):
        """Actually quit the application - FINAL STEP"""
        try:
            logger.info("Quitting application")
            
            # Save window geometry
            self._save_window_state()
            
            # Get the application and quit
            app = self.get_application()
            if app:
                app.quit()
            else:
                # Fallback: close the window directly
                self.close()
                
        except Exception as e:
            logger.error(f"Error during final quit: {e}")
            # Force exit as last resort
            import sys
            sys.exit(0)
        
        return False  # Don't repeat timeout

    def _save_window_state(self):
        """Save window state before quitting"""
        try:
            width, height = self.get_default_size()
            sidebar_width = getattr(self.split_view, 'get_sidebar_width', lambda: 250)()
            self.config.save_window_geometry(width, height, sidebar_width)
            logger.debug(f"Saved window geometry: {width}x{height}, sidebar: {sidebar_width}")
        except Exception as e:
            logger.error(f"Failed to save window state: {e}")
            self.welcome_view.set_visible(False)
            self.tab_view.set_visible(True)
            # Update tab titles in case they've changed
            self._update_tab_titles()
    
    def _update_tab_titles(self):
        """Update tab titles"""
        for page in self.tab_view.get_pages():
            child = page.get_child()
            if hasattr(child, 'connection'):
                page.set_title(child.connection.nickname)
    
    def on_connection_saved(self, dialog, connection_data):
        """Handle connection saved from dialog"""
        try:
            if dialog.is_editing:
                # Update existing connection
                old_connection = dialog.connection
                is_connected = old_connection in self.active_terminals
                
                # Store the current terminal instance if connected
                terminal = self.active_terminals.get(old_connection) if is_connected else None
                
                try:
                    logger.info(
                        "Window.on_connection_saved(edit): saving '%s' with %d forwarding rules",
                        old_connection.nickname, len(connection_data.get('forwarding_rules', []) or [])
                    )
                except Exception:
                    pass
                
                # Detect if anything actually changed; avoid unnecessary writes/prompts
                def _norm_str(v):
                    try:
                        s = ('' if v is None else str(v)).strip()
                        # Treat keyfile placeholders as empty
                        if s.lower().startswith('select key file') or 'select key file or leave empty' in s.lower():
                            return ''
                        return s
                    except Exception:
                        return ''
                def _norm_rules(rules):
                    try:
                        return list(rules or [])
                    except Exception:
                        return []
                existing = {
                    'nickname': _norm_str(getattr(old_connection, 'nickname', '')),
                    'host': _norm_str(getattr(old_connection, 'host', '')),
                    'username': _norm_str(getattr(old_connection, 'username', '')),
                    'port': int(getattr(old_connection, 'port', 22) or 22),
                    'auth_method': int(getattr(old_connection, 'auth_method', 0) or 0),
                    'keyfile': _norm_str(getattr(old_connection, 'keyfile', '')),
                    'key_select_mode': int(getattr(old_connection, 'key_select_mode', 0) or 0),
                    'password': _norm_str(getattr(old_connection, 'password', '')),
                    'key_passphrase': _norm_str(getattr(old_connection, 'key_passphrase', '')),
                    'x11_forwarding': bool(getattr(old_connection, 'x11_forwarding', False)),
                    'forwarding_rules': _norm_rules(getattr(old_connection, 'forwarding_rules', [])),
                    'local_command': _norm_str(getattr(old_connection, 'local_command', '') or (getattr(old_connection, 'data', {}).get('local_command') if hasattr(old_connection, 'data') else '')),
                    'remote_command': _norm_str(getattr(old_connection, 'remote_command', '') or (getattr(old_connection, 'data', {}).get('remote_command') if hasattr(old_connection, 'data') else '')),
                }
                incoming = {
                    'nickname': _norm_str(connection_data.get('nickname')),
                    'host': _norm_str(connection_data.get('host')),
                    'username': _norm_str(connection_data.get('username')),
                    'port': int(connection_data.get('port') or 22),
                    'auth_method': int(connection_data.get('auth_method') or 0),
                    'keyfile': _norm_str(connection_data.get('keyfile')),
                    'key_select_mode': int(connection_data.get('key_select_mode') or 0),
                    'password': _norm_str(connection_data.get('password')),
                    'key_passphrase': _norm_str(connection_data.get('key_passphrase')),
                    'x11_forwarding': bool(connection_data.get('x11_forwarding', False)),
                    'forwarding_rules': _norm_rules(connection_data.get('forwarding_rules')),
                    'local_command': _norm_str(connection_data.get('local_command')),
                    'remote_command': _norm_str(connection_data.get('remote_command')),
                }
                # Determine if anything meaningful changed by comparing canonical SSH config blocks
                try:
                    existing_block = self.connection_manager.format_ssh_config_entry(existing)
                    incoming_block = self.connection_manager.format_ssh_config_entry(incoming)
                    # Also include auth_method/password/key_select_mode delta in change detection
                    pw_changed_flag = bool(connection_data.get('password_changed', False))
                    ksm_changed = (existing.get('key_select_mode', 0) != incoming.get('key_select_mode', 0))
                    changed = (existing_block != incoming_block) or (existing['auth_method'] != incoming['auth_method']) or pw_changed_flag or ksm_changed or (existing['password'] != incoming['password'])
                except Exception:
                    # Fallback to dict comparison if formatter fails
                    changed = existing != incoming

                # Extra guard: if key_select_mode or auth_method differs from the object's current value, force changed
                try:
                    if int(connection_data.get('key_select_mode', -1)) != int(getattr(old_connection, 'key_select_mode', -1)):
                        changed = True
                    if int(connection_data.get('auth_method', -1)) != int(getattr(old_connection, 'auth_method', -1)):
                        changed = True
                except Exception:
                    pass

                if not changed:
                    logger.info("No changes detected for '%s'; skipping update and reconnect prompt", existing['nickname'])
                    # Ensure the UI stays in sync just in case
                    if old_connection in self.connection_rows:
                        self.connection_rows[old_connection].update_display()
                    return

                # Update connection in manager first
                if not self.connection_manager.update_connection(old_connection, connection_data):
                    logger.error("Failed to update connection in SSH config")
                    return
                # Reload from SSH config so UI reflects materialized settings (e.g., IdentitiesOnly)
                try:
                    self.connection_manager.load_ssh_config()
                    self._rebuild_connections_list()
                except Exception:
                    pass
                
                # Update connection attributes in memory (ensure forwarding rules kept)
                old_connection.nickname = connection_data['nickname']
                old_connection.host = connection_data['host']
                old_connection.username = connection_data['username']
                old_connection.port = connection_data['port']
                old_connection.keyfile = connection_data['keyfile']
                old_connection.password = connection_data['password']
                old_connection.key_passphrase = connection_data['key_passphrase']
                old_connection.auth_method = connection_data['auth_method']
                # Persist key selection mode in-memory so the dialog reflects it without restart
                try:
                    old_connection.key_select_mode = int(connection_data.get('key_select_mode', getattr(old_connection, 'key_select_mode', 0)) or 0)
                except Exception:
                    pass
                old_connection.x11_forwarding = connection_data['x11_forwarding']
                old_connection.forwarding_rules = list(connection_data.get('forwarding_rules', []))
                # Update commands
                try:
                    old_connection.local_command = connection_data.get('local_command', '')
                    old_connection.remote_command = connection_data.get('remote_command', '')
                except Exception:
                    pass
                
                # Sync from reloaded manager copy to ensure persistence reflects UI immediately
                try:
                    reloaded = self.connection_manager.find_connection_by_nickname(old_connection.nickname) or \
                               self.connection_manager.find_connection_by_nickname(connection_data.get('nickname', ''))
                    if reloaded:
                        old_connection.forwarding_rules = list(reloaded.forwarding_rules or [])
                        logger.info("Reloaded %d forwarding rules from disk for '%s'", len(old_connection.forwarding_rules), old_connection.nickname)
                except Exception:
                    pass
                
                # Persist per-connection metadata not stored in SSH config (auth method, etc.)
                try:
                    meta_key = old_connection.nickname
                    self.config.set_connection_meta(meta_key, {
                        'auth_method': connection_data.get('auth_method', 0)
                    })
                    # After metadata save, reload config so manager picks up new meta immediately
                    try:
                        self.connection_manager.load_ssh_config()
                        self._rebuild_connections_list()
                    except Exception:
                        pass
                except Exception:
                    pass

                # Update UI
                if old_connection in self.connection_rows:
                    # Get the row before potentially modifying the dictionary
                    row = self.connection_rows[old_connection]
                    # Remove the old connection from the dictionary
                    del self.connection_rows[old_connection]
                    # Add it back with the updated connection object
                    self.connection_rows[old_connection] = row
                    # Update the display
                    row.update_display()
                
                logger.info(f"Updated connection: {old_connection.nickname}")
                
                # If the connection is active, ask if user wants to reconnect
                if is_connected and terminal is not None:
                    # Store the terminal in the connection for later use
                    old_connection._terminal_instance = terminal
                    self._prompt_reconnect(old_connection)
                
            else:
                # Create new connection
                connection = Connection(connection_data)
                # Ensure the in-memory object has the chosen auth_method immediately
                try:
                    connection.auth_method = int(connection_data.get('auth_method', 0))
                except Exception:
                    connection.auth_method = 0
                # Ensure key selection mode is applied immediately
                try:
                    connection.key_select_mode = int(connection_data.get('key_select_mode', 0) or 0)
                except Exception:
                    connection.key_select_mode = 0
                # Add the new connection to the manager's connections list
                self.connection_manager.connections.append(connection)
                
                # Save the connection to SSH config and emit the connection-added signal
                if self.connection_manager.update_connection(connection, connection_data):
                    # Reload from SSH config and rebuild list immediately
                    try:
                        self.connection_manager.load_ssh_config()
                        self._rebuild_connections_list()
                    except Exception:
                        pass
                    # Persist per-connection metadata then reload config
                    try:
                        self.config.set_connection_meta(connection.nickname, {
                            'auth_method': connection_data.get('auth_method', 0)
                        })
                        try:
                            self.connection_manager.load_ssh_config()
                            self._rebuild_connections_list()
                        except Exception:
                            pass
                    except Exception:
                        pass
                    # Sync forwarding rules from a fresh reload to ensure UI matches disk
                    try:
                        reloaded_new = self.connection_manager.find_connection_by_nickname(connection.nickname)
                        if reloaded_new:
                            connection.forwarding_rules = list(reloaded_new.forwarding_rules or [])
                            logger.info("New connection '%s' has %d rules after write", connection.nickname, len(connection.forwarding_rules))
                    except Exception:
                        pass
                    # Manually add the connection to the UI since we're not using the signal
                    # Row list was rebuilt from config; no manual add required
                    logger.info(f"Created new connection: {connection_data['nickname']}")
                else:
                    logger.error("Failed to save connection to SSH config")
                
        except Exception as e:
            logger.error(f"Failed to save connection: {e}")
            # Show error dialog
            error_dialog = Gtk.MessageDialog(
                transient_for=self,
                modal=True,
                message_type=Gtk.MessageType.ERROR,
                buttons=Gtk.ButtonsType.OK,
                text=_("Failed to save connection"),
                secondary_text=str(e)
            )
            error_dialog.present()
    
    def _rebuild_connections_list(self):
        """Rebuild the sidebar connections list from manager state, avoiding duplicates."""
        try:
            # Clear listbox children
            child = self.connection_list.get_first_child()
            while child is not None:
                nxt = child.get_next_sibling()
                self.connection_list.remove(child)
                child = nxt
            # Clear mapping
            self.connection_rows.clear()
            # Re-add from manager
            for conn in self.connection_manager.get_connections():
                self.add_connection_row(conn)
        except Exception:
            pass
    def _prompt_reconnect(self, connection):
        """Show a dialog asking if user wants to reconnect with new settings"""
        dialog = Gtk.MessageDialog(
            transient_for=self,
            modal=True,
            message_type=Gtk.MessageType.QUESTION,
            buttons=Gtk.ButtonsType.YES_NO,
            text=_("Settings Changed"),
            secondary_text=_("The connection settings have been updated.\n"
                           "Would you like to reconnect with the new settings?")
        )
        dialog.connect("response", self._on_reconnect_response, connection)
        dialog.present()
    
    def _on_reconnect_response(self, dialog, response_id, connection):
        """Handle response from reconnect prompt"""
        dialog.destroy()
        
        # Only proceed if user clicked Yes and the connection is still active
        if response_id != Gtk.ResponseType.YES or connection not in self.active_terminals:
            # Clean up the stored terminal instance if it exists
            if hasattr(connection, '_terminal_instance'):
                delattr(connection, '_terminal_instance')
            return
            
        # Get the terminal instance either from active_terminals or the stored instance
        terminal = self.active_terminals.get(connection) or getattr(connection, '_terminal_instance', None)
        if not terminal:
            logger.warning("No terminal instance found for reconnection")
            return
            
        # Set controlled reconnect flag
        self._is_controlled_reconnect = True

        # Show reconnecting feedback
        self._show_reconnecting_message(connection)
        
        try:
            # Disconnect first
            logger.debug("Disconnecting terminal before reconnection")
            terminal.disconnect()
            
            # Store the connection temporarily in active_terminals if not present
            if connection not in self.active_terminals:
                self.active_terminals[connection] = terminal
            
            # Reconnect after a short delay to allow disconnection to complete
            GLib.timeout_add(500, self._reconnect_terminal, connection)
            
        except Exception as e:
            logger.error(f"Error during reconnection: {e}")
            # Remove from active terminals if reconnection fails
            if connection in self.active_terminals:
                del self.active_terminals[connection]
                
            # Show error to user
            error_dialog = Gtk.MessageDialog(
                transient_for=self,
                modal=True,
                message_type=Gtk.MessageType.ERROR,
                buttons=Gtk.ButtonsType.OK,
                text=_("Reconnection Failed"),
                secondary_text=_("Failed to reconnect with the new settings. Please try connecting again manually.")
            )
            error_dialog.present()
            
        finally:
            # Clean up the stored terminal instance
            if hasattr(connection, '_terminal_instance'):
                delattr(connection, '_terminal_instance')
                
            # Reset the flag after a delay to ensure it's not set during normal operations
            GLib.timeout_add(1000, self._reset_controlled_reconnect)
    
    def _reset_controlled_reconnect(self):
        """Reset the controlled reconnect flag"""
        self._is_controlled_reconnect = False
    
    def _reconnect_terminal(self, connection):
        """Reconnect a terminal with updated connection settings"""
        if connection not in self.active_terminals:
            logger.warning(f"Connection {connection.nickname} not found in active terminals")
            return False  # Don't repeat the timeout
            
        terminal = self.active_terminals[connection]
        
        try:
            logger.debug(f"Attempting to reconnect terminal for {connection.nickname}")
            
            # Reconnect with new settings
            if not terminal._connect_ssh():
                logger.error("Failed to reconnect with new settings")
                # Show error to user
                GLib.idle_add(self._show_reconnect_error, connection)
                return False
                
            logger.info(f"Successfully reconnected terminal for {connection.nickname}")
            
        except Exception as e:
            logger.error(f"Error reconnecting terminal: {e}", exc_info=True)
            GLib.idle_add(self._show_reconnect_error, connection, str(e))
            
        return False  # Don't repeat the timeout
        
    def _show_reconnect_error(self, connection, error_message=None):
        """Show an error message when reconnection fails"""
        # Ensure reconnecting feedback is hidden
        self._hide_reconnecting_message()
        # Remove from active terminals if reconnection fails
        if connection in self.active_terminals:
            del self.active_terminals[connection]
            
        # Update UI to show disconnected state
        if connection in self.connection_rows:
            self.connection_rows[connection].update_status()
        
        # Show error dialog
        error_dialog = Gtk.MessageDialog(
            transient_for=self,
            modal=True,
            message_type=Gtk.MessageType.ERROR,
            buttons=Gtk.ButtonsType.OK,
            text=_("Reconnection Failed"),
            secondary_text=error_message or _("Failed to reconnect with the new settings. Please try connecting again manually.")
        )
        error_dialog.present()
        
        # Clean up the dialog when closed
        error_dialog.connect("response", lambda d, r: d.destroy())