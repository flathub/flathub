#!/usr/bin/env python3
import gi
gi.require_version('Gtk', '4.0')
gi.require_version('Adw', '1')
from gi.repository import Gtk, Adw, GLib, Gio, Gdk, Pango
import subprocess
import os
from datetime import datetime

APP_VERSION = "1.2.0"

class SystemdManagerWindow(Adw.ApplicationWindow):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.set_default_size(800, 600)
        self.set_title("systemd Pilot")
        self.all_services = []
        self.is_root = os.geteuid() == 0
        self.current_filter = "all"  # Track current filter

        # Set up search action
        search_action = Gio.SimpleAction.new("search", None)
        search_action.connect("activate", self.toggle_search)
        self.add_action(search_action)

        # Main layout
        self.main_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.set_content(self.main_box)

        # Header bar
        header = Adw.HeaderBar()
        self.main_box.append(header)

        # Search button
        self.search_button = Gtk.ToggleButton(icon_name="system-search-symbolic")
        self.search_button.set_tooltip_text("Search services (Ctrl+F)")
        self.search_button.connect("toggled", self.on_search_toggled)
        header.pack_end(self.search_button)

        # Menu button
        menu_button = Gtk.MenuButton()
        menu_button.set_icon_name("open-menu-symbolic")
        menu_button.set_tooltip_text("Main menu")
        header.pack_end(menu_button)

        # Create menu
        menu = Gio.Menu()
        menu.append("Reload Configuration", "app.reload")
        menu.append("Feedback", "app.feedback")
        menu.append("About", "app.about")
        menu_button.set_menu_model(menu)

        # Create filter buttons in a ribbon
        filter_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        filter_box.add_css_class("toolbar")
        filter_box.set_margin_start(6)
        filter_box.set_margin_end(6)
        filter_box.set_margin_top(6)
        filter_box.set_margin_bottom(6)

        # All services filter
        all_button = Gtk.ToggleButton(label="All")
        all_button.set_active(True)
        all_button.connect("toggled", self.on_filter_changed, "all")
        filter_box.append(all_button)

        # Running services filter
        running_button = Gtk.ToggleButton(label="Running")
        running_button.connect("toggled", self.on_filter_changed, "running")
        filter_box.append(running_button)

        # Inactive services filter
        inactive_button = Gtk.ToggleButton(label="Inactive")
        inactive_button.connect("toggled", self.on_filter_changed, "inactive")
        filter_box.append(inactive_button)

        # Failed services filter
        failed_button = Gtk.ToggleButton(label="Failed")
        failed_button.connect("toggled", self.on_filter_changed, "failed")
        filter_box.append(failed_button)

        # User services filter (moved to end)
        user_button = Gtk.ToggleButton(label="User")
        user_button.connect("toggled", self.on_filter_changed, "user")
        filter_box.append(user_button)

        # Store filter buttons for toggling
        self.filter_buttons = {
            "all": all_button,
            "running": running_button,
            "inactive": inactive_button,
            "failed": failed_button,
            "user": user_button
        }

        self.main_box.append(filter_box)



        # Search bar
        self.search_bar = Gtk.SearchBar()
        self.search_entry = Gtk.SearchEntry()
        self.search_entry.set_hexpand(True)
        self.search_entry.connect("search-changed", self.on_search_changed)
        self.search_bar.set_child(self.search_entry)
        self.search_bar.set_key_capture_widget(self)
        self.search_bar.connect_entry(self.search_entry)
        self.main_box.append(self.search_bar)

        # Create scrolled window
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_vexpand(True)
        self.main_box.append(scrolled)

        # Create list box for services
        self.list_box = Gtk.ListBox()
        self.list_box.set_selection_mode(Gtk.SelectionMode.NONE)
        self.list_box.add_css_class("boxed-list")
        self.list_box.set_filter_func(self.filter_services)
        scrolled.set_child(self.list_box)

        # Add loading spinner
        self.spinner_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        self.spinner_box.set_valign(Gtk.Align.CENTER)
        self.spinner_box.set_vexpand(True)
        
        self.spinner = Gtk.Spinner()
        self.spinner.set_size_request(32, 32)
        self.spinner_box.append(self.spinner)
        
        loading_label = Gtk.Label(label="Loading services...")
        self.spinner_box.append(loading_label)
        
        self.list_box.append(self.spinner_box)
        self.spinner.start()

        # Load services after window is shown
        GLib.idle_add(self.load_services)

        # Add CSS provider
        css_provider = Gtk.CssProvider()
        css_provider.load_from_data(b"""
            .dark {
                background-color: #303030;
                border-radius: 6px;
                padding: 6px;
            }
            .white {
                color: white;
            }
            .dark-button {
                background: #1a1a1a;
                color: white;
                border: none;
                box-shadow: none;
                text-shadow: none;
                -gtk-icon-shadow: none;
                outline: none;
                border-radius: 4px;
                padding: 8px 12px;
                min-height: 0;
            }
            .dark-button:hover {
                background: #2a2a2a;
            }
            .dark-button:active {
                background: #000000;
            }
            row {
                padding: 6px;
            }
            .expander-row {
                padding: 6px;
            }
            .expander-row > box > label {
                font-weight: bold;
                font-size: 1.2em;
            }
            .service-active {
                color: #73d216;
            }
            .service-inactive {
                color: #cc0000;
            }
        """)
        
        Gtk.StyleContext.add_provider_for_display(
            Gdk.Display.get_default(),
            css_provider,
            Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )

    @staticmethod
    def is_running_in_flatpak():
        """Check if the application is running inside Flatpak"""
        return os.path.exists("/.flatpak-info")

    def run_host_command(self, cmd):
        """Run a command on the host system, handling Flatpak if needed"""
        if SystemdManagerWindow.is_running_in_flatpak():
            return ["flatpak-spawn", "--host"] + cmd
        return cmd

    def load_services(self):
        """Load systemd services based on current filter"""
        try:
            if self.spinner_box.get_parent():
                self.list_box.remove(self.spinner_box)

            # For user filter, show all user services
            if self.current_filter == "user":
                cmd = ["systemctl", "--user", "list-units", "--type=service", "--all", "--no-pager", "--plain"]
                output = subprocess.run(
                    self.run_host_command(cmd),
                    capture_output=True,
                    text=True,
                    check=True
                ).stdout
            else:
                # For all other filters, combine system and user services
                # Get system services
                system_cmd = ["systemctl", "list-units", "--type=service"]
                if self.current_filter == "all":
                    system_cmd.append("--all")
                elif self.current_filter == "failed":
                    system_cmd.extend(["--state=failed"])
                elif self.current_filter == "running":
                    system_cmd.extend(["--state=running"])
                elif self.current_filter == "inactive":
                    system_cmd.extend(["--state=inactive"])
                system_cmd.extend(["--no-pager", "--plain"])
                
                system_output = subprocess.run(
                    self.run_host_command(system_cmd),
                    capture_output=True,
                    text=True,
                    check=True
                ).stdout

                # Get user services
                user_cmd = ["systemctl", "--user", "list-units", "--type=service"]
                if self.current_filter == "all":
                    user_cmd.append("--all")
                elif self.current_filter == "failed":
                    user_cmd.extend(["--state=failed"])
                elif self.current_filter == "running":
                    user_cmd.extend(["--state=running"])
                elif self.current_filter == "inactive":
                    user_cmd.extend(["--state=inactive"])
                user_cmd.extend(["--no-pager", "--plain"])
                
                user_output = subprocess.run(
                    self.run_host_command(user_cmd),
                    capture_output=True,
                    text=True,
                    check=True
                ).stdout

                # Combine outputs
                output = system_output + "\n" + user_output

            self.parse_systemctl_output(output)
            
        except subprocess.CalledProcessError as e:
            print(f"Error loading services: {e}")
            self.show_error_dialog("Failed to load service information")

    def parse_systemctl_output(self, output):
        """Parse systemctl output"""
        services = []
        for line in output.splitlines()[1:]:  # Skip header line
            if not line.strip():
                continue
                
            parts = line.split(maxsplit=4)
            if len(parts) >= 4:
                unit_name = parts[0]
                if unit_name.endswith('.service'):
                    service_data = {
                        'name': unit_name[:-8],  # Remove '.service' suffix
                        'full_name': unit_name,  # Keep full name for systemctl commands
                        'load': parts[1],
                        'active': parts[2],
                        'sub': parts[3],
                        'description': parts[4] if len(parts) > 4 else ''
                    }
                    services.append(service_data)

        self.all_services = services
        self.refresh_display()

    def create_service_row(self, service_data):
        """Create a row for a service"""
        row = Adw.ExpanderRow()
        
        # Set the service name as title
        row.set_title(service_data['name'])
        
        # Set the status as subtitle
        status_class = "service-active" if service_data['active'] == "active" else "service-inactive"
        status_text = f"{service_data['active']} ({service_data['sub']})"
        row.set_subtitle(status_text)

        # Details box
        details_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        details_box.set_margin_start(12)
        details_box.set_margin_end(12)
        details_box.set_margin_top(6)
        details_box.set_margin_bottom(6)
        details_box.add_css_class("dark")

        # Add service details
        def create_detail_label(text):
            # Special handling for running state in details
            if "Sub-state: running" in text:
                prefix, _ = text.split("running", 1)
                label = Gtk.Label(xalign=0)
                label.set_markup(f"{prefix}<span foreground='#73d216'>running</span>")
            else:
                label = Gtk.Label(label=text, xalign=0)
            
            label.set_wrap(True)
            label.set_wrap_mode(Pango.WrapMode.WORD_CHAR)
            label.set_hexpand(True)
            label.add_css_class("white")
            return label

        details_box.append(create_detail_label(f"Description: {service_data['description']}"))
        details_box.append(create_detail_label(f"Load: {service_data['load']}"))
        details_box.append(create_detail_label(f"Active: {service_data['active']}"))
        details_box.append(create_detail_label(f"Sub-state: {service_data['sub']}"))



        # Add action buttons
        buttons_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        buttons_box.set_margin_top(6)
        
        status_button = Gtk.Button(label="Status")
        status_button.set_tooltip_text("Show detailed service status")
        status_button.connect("clicked", self.on_show_status, service_data['name'])
        status_button.add_css_class("dark-button")
        buttons_box.append(status_button)

        start_button = Gtk.Button(label="Start")
        start_button.connect("clicked", self.on_start_service, service_data['name'])
        start_button.add_css_class("dark-button")
        buttons_box.append(start_button)

        stop_button = Gtk.Button(label="Stop")
        stop_button.connect("clicked", self.on_stop_service, service_data['name'])
        stop_button.add_css_class("dark-button")
        buttons_box.append(stop_button)

        restart_button = Gtk.Button(label="Restart")
        restart_button.connect("clicked", self.on_restart_service, service_data['name'])
        restart_button.add_css_class("dark-button")
        buttons_box.append(restart_button)

        enable_button = Gtk.Button(label="Enable")
        enable_button.connect("clicked", self.on_enable_service, service_data['name'])
        enable_button.add_css_class("dark-button")
        buttons_box.append(enable_button)

        disable_button = Gtk.Button(label="Disable")
        disable_button.connect("clicked", self.on_disable_service, service_data['name'])
        disable_button.add_css_class("dark-button")
        buttons_box.append(disable_button)

        edit_button = Gtk.Button(label="Edit")
        edit_button.set_tooltip_text("Override settings for this unit")
        edit_button.connect("clicked", self.on_edit_service, service_data['name'])
        edit_button.add_css_class("dark-button")
        buttons_box.append(edit_button)

        details_box.append(buttons_box)

        # Add details to row
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.NEVER)
        scrolled.set_child(details_box)
        row.add_row(scrolled)

        return row

    def run_systemctl_command(self, command, service_name):
        """Run a systemctl command with pkexec if needed"""
        try:
            # Get the row and its expanded state
            row = None
            scrolled_window = None
            scroll_value = None
            
            # Find the scrolled window
            for widget in self.main_box:
                if isinstance(widget, Gtk.ScrolledWindow):
                    scrolled_window = widget
                    vadjustment = scrolled_window.get_vadjustment()
                    scroll_value = vadjustment.get_value()
                    break
            
            # Find the row
            for child in self.list_box.observe_children():
                if child.get_title() == service_name:
                    row = child
                    break
            
            was_expanded = row.get_expanded() if row else False
            
            # Check if this is a user service by listing all user services
            is_user_service = self.check_if_user_service(service_name)
            
            service_name = f"{service_name}.service"  # Add .service suffix
            
            # Build command based on service type
            if is_user_service:
                cmd = ["systemctl", "--user", command, service_name]
            else:
                cmd = ["systemctl", command, service_name]
                if not self.is_root:
                    cmd.insert(0, "pkexec")
            
            # Run command with Flatpak handling
            subprocess.run(self.run_host_command(cmd), check=True)
            
            # Use a callback to refresh all services but keep the current row expanded and scroll position
            def refresh_and_restore():
                self.refresh_data()
                # Find the row again after refresh and expand it if it was expanded
                if was_expanded:
                    for child in self.list_box.observe_children():
                        if child.get_title() == service_name[:-8]:  # Remove .service suffix
                            child.set_expanded(True)
                            # Restore scroll position
                            if scrolled_window and scroll_value is not None:
                                GLib.idle_add(lambda: scrolled_window.get_vadjustment().set_value(scroll_value))
                            break
                return False
            
            GLib.timeout_add(1000, refresh_and_restore)
            
        except subprocess.CalledProcessError as e:
            self.show_error_dialog(f"Failed to {command} service: {e}")

    def on_start_service(self, button, service_name):
        self.run_systemctl_command("start", service_name)

    def on_stop_service(self, button, service_name):
        self.run_systemctl_command("stop", service_name)

    def on_restart_service(self, button, service_name):
        self.run_systemctl_command("restart", service_name)

    def on_enable_service(self, button, service_name):
        self.run_systemctl_command("enable", service_name)

    def on_disable_service(self, button, service_name):
        self.run_systemctl_command("disable", service_name)

    def on_edit_service(self, button, service_name):
        """Open systemctl edit for the service"""
        try:
            # Get the row and its expanded state
            row = None
            for child in self.list_box.observe_children():
                if child.get_title() == service_name:
                    row = child
                    break
            
            was_expanded = row.get_expanded() if row else False
            
            # Check if this is a user service
            is_user_service = self.check_if_user_service(service_name)
            service_name = f"{service_name}.service"
            
            # Build the edit command based on service type
            if is_user_service:
                edit_cmd = f"systemctl --user edit {service_name}; read -p 'Press Enter to close...'"
            else:
                edit_cmd = f"pkexec systemctl edit {service_name}; read -p 'Press Enter to close...'"
            
            terminal = self.get_terminal_command()
            if terminal is None:
                self.show_error_dialog("No suitable terminal emulator found. Please install gnome-terminal, xfce4-terminal, or konsole.")
                return
            
            # Build the complete command
            terminal_cmd = [terminal['binary']]
            terminal_cmd.extend(terminal['args'])
            terminal_cmd.append(edit_cmd)
            
            # Use run_host_command for the complete command
            cmd = self.run_host_command(terminal_cmd)
            
            GLib.spawn_async(
                argv=cmd,
                flags=GLib.SpawnFlags.SEARCH_PATH | GLib.SpawnFlags.DO_NOT_REAP_CHILD,
                child_setup=None,
                user_data=None
            )
            
            # Use a callback to restore expanded state after edit
            def restore_expanded_state():
                if was_expanded:
                    for child in self.list_box.observe_children():
                        if child.get_title() == service_name[:-8]:
                            child.set_expanded(True)
                            break
                return False
            
            GLib.timeout_add(1000, restore_expanded_state)
            
        except GLib.Error as e:
            self.show_error_dialog(f"Failed to edit service: {e.message}")

    def refresh_data(self, *args):
        """Refresh the service data"""
        self.load_services()

    def on_search_toggled(self, button):
        self.search_bar.set_search_mode(button.get_active())

    def on_search_changed(self, entry):
        self.list_box.invalidate_filter()

    def filter_services(self, row):
        """Filter services based on search text and current filter"""
        if not hasattr(row, 'get_title'):
            return True

        # First apply search filter
        show_by_search = True
        if self.search_entry.get_text():
            search_text = self.search_entry.get_text().lower()
            title = row.get_title().lower()
            subtitle = row.get_subtitle().lower()
            show_by_search = search_text in title or search_text in subtitle

        # Then apply status filter
        show_by_status = True
        if self.current_filter != "all":
            subtitle = row.get_subtitle().lower()
            if self.current_filter == "running":
                show_by_status = "running" in subtitle
            elif self.current_filter == "inactive":
                show_by_status = "inactive" in subtitle
            elif self.current_filter == "failed":
                show_by_status = "failed" in subtitle

        return show_by_search and show_by_status

    def show_error_dialog(self, message):
        dialog = Adw.MessageDialog(
            parent=self,
            heading="Error",
            body=message
        )
        dialog.add_response("ok", "_OK")
        dialog.present()

    def refresh_display(self):
        """Update the display with the current service data"""
        while True:
            row = self.list_box.get_first_child()
            if row is None:
                break
            self.list_box.remove(row)

        for service_data in self.all_services:
            row = self.create_service_row(service_data)
            self.list_box.append(row)

    def toggle_search(self, action, param):
        self.search_button.set_active(not self.search_button.get_active())

    def on_filter_changed(self, button, filter_type):
        """Handle filter button toggles"""
        if button.get_active():
            # Deactivate other filter buttons
            for btn_type, btn in self.filter_buttons.items():
                if btn_type != filter_type:
                    btn.set_active(False)
            
            self.current_filter = filter_type
            self.refresh_data()  # Reload with new filter

    def on_daemon_reload(self, button):
        """Reload systemd daemon configuration"""
        try:
            cmd = ["systemctl", "daemon-reload"]
            if not self.is_root:
                cmd.insert(0, "pkexec")
            
            subprocess.run(cmd, check=True)
            self.refresh_data()  # Refresh the service list
            
        except subprocess.CalledProcessError as e:
            self.show_error_dialog(f"Failed to reload daemon: {e}")

    def on_show_status(self, button, service_name):
        """Show detailed status of the service"""
        try:
            # Get the row and its expanded state
            row = None
            for child in self.list_box.observe_children():
                if child.get_title() == service_name:
                    row = child
                    break
            
            was_expanded = row.get_expanded() if row else False
            
            # Check if this is a user service
            is_user_service = self.check_if_user_service(service_name)
            service_name = f"{service_name}.service"
            
            # Build the status command based on service type
            status_cmd = f"systemctl {'--user ' if is_user_service else ''}status {service_name}; read -p 'Press Enter to close...'"
            
            terminal = self.get_terminal_command()
            if terminal is None:
                self.show_error_dialog("No suitable terminal emulator found. Please install gnome-terminal, xfce4-terminal, or konsole.")
                return
            
            # Build the complete command
            terminal_cmd = [terminal['binary']]
            terminal_cmd.extend(terminal['args'])
            terminal_cmd.append(status_cmd)
            
            # Use run_host_command for the complete command
            cmd = self.run_host_command(terminal_cmd)
            
            GLib.spawn_async(
                argv=cmd,
                flags=GLib.SpawnFlags.SEARCH_PATH | GLib.SpawnFlags.DO_NOT_REAP_CHILD,
                child_setup=None,
                user_data=None
            )
            
            # Use a callback to restore expanded state after showing status
            def restore_expanded_state():
                if was_expanded:
                    for child in self.list_box.observe_children():
                        if child.get_title() == service_name[:-8]:
                            child.set_expanded(True)
                            break
                return False
            
            GLib.timeout_add(1000, restore_expanded_state)
            
        except GLib.Error as e:
            self.show_error_dialog(f"Failed to show service status: {e.message}")

    def check_if_user_service(self, service_name):
        """Helper method to check if a service is a user service"""
        try:
            check_cmd = ["systemctl", "--user", "list-units", "--type=service", "--all", "--no-pager", "--plain"]
            result = subprocess.run(
                self.run_host_command(check_cmd),
                capture_output=True,
                text=True,
                check=True
            )
            # Check if the service name appears in the user services list
            return any(service_name in line for line in result.stdout.splitlines())
        except subprocess.CalledProcessError:
            return False

    def get_terminal_command(self):
        """Helper function to find an available terminal emulator"""
        terminals = [
            {
                'binary': 'gnome-terminal',
                'args': ['--', 'bash', '-c']
            },
            {
                'binary': 'xfce4-terminal',
                'args': ['-e', 'bash -c']
            },
            {
                'binary': 'konsole',
                'args': ['-e', 'bash -c']
            },
            {
                'binary': 'x-terminal-emulator',
                'args': ['-e', 'bash -c']
            }
        ]
        
        for terminal in terminals:
            try:
                # Use run_host_command to check if terminal exists
                subprocess.run(
                    self.run_host_command(['which', terminal['binary']]),
                    check=True,
                    capture_output=True
                )
                return terminal
            except subprocess.CalledProcessError:
                continue
        return None

class SystemdManagerApp(Adw.Application):
    def __init__(self):
        super().__init__(application_id="org.mfat.systemdpilot",
                        flags=Gio.ApplicationFlags.FLAGS_NONE)
        self.connect('activate', self.on_activate)
        self.connect('shutdown', self.on_shutdown)
        
        self.set_accels_for_action("win.search", ["<Control>f"])
        
        # Add reload action
        reload_action = Gio.SimpleAction.new("reload", None)
        reload_action.connect("activate", self.on_reload_action)
        self.add_action(reload_action)
        
        # Add feedback action
        feedback_action = Gio.SimpleAction.new("feedback", None)
        feedback_action.connect("activate", self.on_feedback_action)
        self.add_action(feedback_action)
        
        about_action = Gio.SimpleAction.new("about", None)
        about_action.connect("activate", self.on_about_action)
        self.add_action(about_action)

    def on_activate(self, app):
        win = SystemdManagerWindow(application=app)
        win.present()

    def on_shutdown(self, app):
        for window in self.get_windows():
            window.close()

    def on_about_action(self, action, param):
        about = Adw.AboutWindow(
            transient_for=self.get_active_window(),
            application_name="systemd Pilot",
            application_icon="system-run",
            developer_name="mFat",
            version=APP_VERSION,
            website="https://github.com/mfat/systemd-pilot",
            license_type=Gtk.License.GPL_3_0,
            developers=["mFat"],
            copyright="© 2024 mFat"
        )
        about.present()

    def on_reload_action(self, action, param):
        """Handle reload action from menu"""
        active_window = self.get_active_window()
        if active_window:
            active_window.on_daemon_reload(None)

    def on_feedback_action(self, action, param):
        """Open feedback URL in default browser"""
        Gtk.show_uri(
            self.get_active_window(),
            "https://github.com/mfat/systemd-pilot/issues",
            Gdk.CURRENT_TIME
        )

app = SystemdManagerApp()
app.run(None)