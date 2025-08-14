"""
Terminal Widget for sshPilot
Integrated VTE terminal with SSH connection handling using system SSH client
"""

import os
import sys
import logging
import signal
import time
import json
import re
import gi
from gettext import gettext as _
import asyncio
import threading
import weakref
import subprocess
import shutil
from datetime import datetime

gi.require_version('Gtk', '4.0')
gi.require_version('Vte', '3.91')

gi.require_version('Adw', '1')
from gi.repository import Gtk, GObject, GLib, Vte, Pango, Gdk, Gio, Adw

logger = logging.getLogger(__name__)

class SSHProcessManager:
    """Manages SSH processes and ensures proper cleanup"""
    _instance = None
    
    def __new__(cls):
        if cls._instance is None:
            cls._instance = super().__new__(cls)
            cls._instance.processes = {}
            cls._instance.terminals = weakref.WeakSet()
            cls._instance.lock = threading.Lock()
            cls._instance.cleanup_thread = None
            cls._instance._start_cleanup_thread()
        return cls._instance
    
    def _start_cleanup_thread(self):
        """Start background cleanup thread"""
        if self.cleanup_thread is None or not self.cleanup_thread.is_alive():
            self.cleanup_thread = threading.Thread(target=self._cleanup_loop, daemon=True)
            self.cleanup_thread.start()
            logger.debug("Started SSH cleanup thread")
    
    def _cleanup_loop(self):
        """Background cleanup loop"""
        while True:
            try:
                time.sleep(30)
                self._cleanup_orphaned_processes()
            except Exception as e:
                logger.error(f"Error in cleanup loop: {e}")
    
    def _cleanup_orphaned_processes(self):
        """Clean up processes not tracked by active terminals"""
        with self.lock:
            active_pids = set()
            for terminal in list(self.terminals):
                try:
                    pid = terminal._get_terminal_pid()
                    if pid:
                        active_pids.add(pid)
                except Exception as e:
                    logger.error(f"Error getting PID from terminal: {e}")
            
            for pid in list(self.processes.keys()):
                if pid not in active_pids:
                    self._terminate_process_by_pid(pid)
    
    def _terminate_process_by_pid(self, pid):
        """Terminate a process by PID"""
        try:
            pgid = os.getpgid(pid)
            os.killpg(pgid, signal.SIGTERM)
            time.sleep(1)
            try:
                os.kill(pid, 0)
                os.killpg(pgid, signal.SIGKILL)
            except ProcessLookupError:
                pass
        except (ProcessLookupError, OSError) as e:
            logger.debug(f"Process {pid} cleanup: {e}")
        finally:
            with self.lock:
                if pid in self.processes:
                    del self.processes[pid]
    
    def register_terminal(self, terminal):
        """Register a terminal for tracking"""
        self.terminals.add(terminal)
        logger.debug(f"Registered terminal {id(terminal)}")
    
    def cleanup_all(self):
        """Clean up all managed processes"""
        logger.info("Cleaning up all SSH processes...")
        with self.lock:
            # Make a copy of PIDs to avoid modifying the dict during iteration
            pids = list(self.processes.keys())
            for pid in pids:
                self._terminate_process_by_pid(pid)
            
            # Clear all tracked processes
            self.processes.clear()
            
            # Clean up any remaining terminals
            for terminal in list(self.terminals):
                try:
                    if hasattr(terminal, 'disconnect'):
                        terminal.disconnect()
                except Exception as e:
                    logger.error(f"Error cleaning up terminal {id(terminal)}: {e}")
            
            # Clear terminal references
            self.terminals.clear()
            
        logger.info("SSH process cleanup completed")

# Global process manager instance
process_manager = SSHProcessManager()

class TerminalWidget(Gtk.Box):
    """A terminal widget that uses VTE for display and system SSH client for connections"""
    __gtype_name__ = 'TerminalWidget'
    
    # Signals
    __gsignals__ = {
        'connection-established': (GObject.SignalFlags.RUN_FIRST, None, ()),
        'connection-failed': (GObject.SignalFlags.RUN_FIRST, None, (str,)),
        'connection-lost': (GObject.SignalFlags.RUN_FIRST, None, ()),
        'title-changed': (GObject.SignalFlags.RUN_FIRST, None, (str,)),
    }
    
    def __init__(self, connection, config, connection_manager):
        # Initialize as a vertical Gtk.Box
        super().__init__(orientation=Gtk.Orientation.VERTICAL)
        
        # Store references
        self.connection = connection
        self.config = config
        self.connection_manager = connection_manager
        
        # Process tracking
        self.process = None
        self.process_pid = None
        self.process_pgid = None
        self.is_connected = False
        self.watch_id = 0
        self.ssh_client = None
        self.session_id = str(id(self))  # Unique ID for this session
        
        # Register with process manager
        process_manager.register_terminal(self)
        
        # Connect to signals
        self.connect('destroy', self._on_destroy)
        
        # Connect to connection manager signals using GObject.GObject.connect directly
        self._connection_updated_handler = GObject.GObject.connect(connection_manager, 'connection-updated', self._on_connection_updated_signal)
        logger.debug("Connected to connection-updated signal")
        
        # Create scrolled window for terminal
        self.scrolled_window = Gtk.ScrolledWindow()
        self.scrolled_window.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        
        # Set up the terminal
        self.vte = Vte.Terminal()
        
        # Initialize terminal with basic settings and apply configured theme early
        self.setup_terminal()
        try:
            self.apply_theme()
        except Exception:
            pass
        
        # Add terminal to scrolled window and to the box via an overlay with a connecting view
        self.scrolled_window.set_child(self.vte)
        self.overlay = Gtk.Overlay()
        self.overlay.set_child(self.scrolled_window)

        # Connecting overlay elements
        self.connecting_bg = Gtk.Box()
        self.connecting_bg.set_hexpand(True)
        self.connecting_bg.set_vexpand(True)
        try:
            provider = Gtk.CssProvider()
            provider.load_from_data(b".connecting-bg { background-color: #000000; }")
            display = Gdk.Display.get_default()
            if display:
                Gtk.StyleContext.add_provider_for_display(display, provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
            if hasattr(self.connecting_bg, 'add_css_class'):
                self.connecting_bg.add_css_class('connecting-bg')
        except Exception:
            pass

        self.connecting_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        self.connecting_box.set_halign(Gtk.Align.CENTER)
        self.connecting_box.set_valign(Gtk.Align.CENTER)
        spinner = Gtk.Spinner()
        spinner.start()
        label = Gtk.Label()
        label.set_markup('<span color="#FFFFFF">Connecting</span>')
        self.connecting_box.append(spinner)
        self.connecting_box.append(label)

        self.overlay.add_overlay(self.connecting_bg)
        self.overlay.add_overlay(self.connecting_box)

        # Disconnected banner with reconnect button at the bottom (separate panel below terminal)
        # Install CSS for a solid red background banner once
        try:
            display = Gdk.Display.get_default()
            if display and not getattr(display, '_sshpilot_banner_css_installed', False):
                css_provider = Gtk.CssProvider()
                css_provider.load_from_data(b"""
                    .error-toolbar.toolbar {
                        background-color: #cc0000;
                        color: #ffffff;
                        border-radius: 0;
                        padding-top: 10px;
                        padding-bottom: 10px;
                    }
                    .error-toolbar.toolbar label { color: #ffffff; }
                    .reconnect-button { background: #4a4a4a; color: #ffffff; border-radius: 4px; padding: 6px 10px; }
                    .reconnect-button:hover { background: #3f3f3f; }
                    .reconnect-button:active { background: #353535; }
                """)
                Gtk.StyleContext.add_provider_for_display(
                    display, css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
                )
                setattr(display, '_sshpilot_banner_css_installed', True)
        except Exception:
            pass

        # Create error toolbar with same structure as sidebar toolbar
        self.disconnected_banner = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        self.disconnected_banner.set_halign(Gtk.Align.FILL)
        self.disconnected_banner.set_valign(Gtk.Align.END)
        self.disconnected_banner.set_margin_start(0)
        self.disconnected_banner.set_margin_end(0)
        self.disconnected_banner.set_margin_top(0)
        self.disconnected_banner.set_margin_bottom(0)
        try:
            self.disconnected_banner.add_css_class('toolbar')
            self.disconnected_banner.add_css_class('error-toolbar')
            # Add a unique class per instance so we can set a per-widget min-height via CSS
            self._banner_unique_class = f"banner-{id(self)}"
            self.disconnected_banner.add_css_class(self._banner_unique_class)
        except Exception:
            pass
        # Banner content: icon + label + spacer + reconnect + dismiss, matching toolbar layout
        icon = Gtk.Image.new_from_icon_name('dialog-error-symbolic')
        icon.set_valign(Gtk.Align.CENTER)
        self.disconnected_banner.append(icon)
        self.disconnected_banner_label = Gtk.Label()
        self.disconnected_banner_label.set_halign(Gtk.Align.START)
        self.disconnected_banner_label.set_valign(Gtk.Align.CENTER)
        self.disconnected_banner_label.set_hexpand(True)
        self.disconnected_banner_label.set_text(_('Session ended.'))
        self.disconnected_banner.append(self.disconnected_banner_label)
        self.reconnect_button = Gtk.Button.new_with_label(_('Reconnect'))
        try:
            self.reconnect_button.add_css_class('reconnect-button')
        except Exception:
            pass
        self.reconnect_button.connect('clicked', self._on_reconnect_clicked)
        self.disconnected_banner.append(self.reconnect_button)

        # Dismiss button to hide the banner manually
        self.dismiss_button = Gtk.Button.new_with_label(_('Dismiss'))
        try:
            self.dismiss_button.add_css_class('flat')
            self.dismiss_button.add_css_class('reconnect-button')
        except Exception:
            pass
        self.dismiss_button.connect('clicked', lambda *_: self._set_disconnected_banner_visible(False))
        self.disconnected_banner.append(self.dismiss_button)
        self.disconnected_banner.set_visible(False)

        # Allow window to force an exact height match to the sidebar toolbar using per-widget CSS min-height
        self._banner_css_provider = None
        def _apply_external_height(new_h: int):
            try:
                h = max(0, int(new_h))
                display = Gdk.Display.get_default()
                if not display:
                    return
                css = f".{self._banner_unique_class} {{ min-height: {h}px; }}"
                provider = Gtk.CssProvider()
                provider.load_from_data(css.encode('utf-8'))
                Gtk.StyleContext.add_provider_for_display(display, provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
                # Keep a reference to prevent GC; latest provider wins at same priority
                self._banner_css_provider = provider
            except Exception:
                pass
        self.set_banner_height = _apply_external_height

        # Container to stack terminal (overlay) above the banner panel
        self.container_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.container_box.set_hexpand(True)
        self.container_box.set_vexpand(True)
        self.container_box.append(self.overlay)
        self.container_box.append(self.disconnected_banner)

        self.append(self.container_box)
        
        # Set expansion properties
        self.scrolled_window.set_hexpand(True)
        self.scrolled_window.set_vexpand(True)
        self.vte.set_hexpand(True)
        self.vte.set_vexpand(True)
        
        # Connect terminal signals
        self.vte.connect('child-exited', self.on_child_exited)
        self.vte.connect('window-title-changed', self.on_title_changed)
        
        # Apply theme
        self.force_style_refresh()
        
        # Set visibility of child widgets (GTK4 style)
        self.scrolled_window.set_visible(True)
        self.vte.set_visible(True)
        
        # Show overlay initially
        self._set_connecting_overlay_visible(True)
        logger.debug("Terminal widget initialized")

    def _set_disconnected_banner_visible(self, visible: bool, message: str = None):
        try:
            # Allow callers (e.g., ssh-copy-id dialog) to suppress the red banner entirely
            if getattr(self, '_suppress_disconnect_banner', False):
                return
            if message:
                self.disconnected_banner_label.set_text(message)
            if hasattr(self.disconnected_banner, 'set_visible'):
                self.disconnected_banner.set_visible(visible)
        except Exception:
            pass

    def _on_reconnect_clicked(self, *args):
        """User clicked reconnect on the banner"""
        try:
            # Immediately hide banner and show connecting overlay
            self._set_disconnected_banner_visible(False)
            self._set_connecting_overlay_visible(True)
            # Reuse existing connect method
            if not self._connect_ssh():
                # Show banner again if failed to start reconnect
                self._set_connecting_overlay_visible(False)
                self._set_disconnected_banner_visible(True, _('Reconnect failed to start'))
        except Exception:
            self._set_connecting_overlay_visible(False)
            self._set_disconnected_banner_visible(True, _('Reconnect failed'))

    def _set_connecting_overlay_visible(self, visible: bool):
        try:
            if hasattr(self.connecting_bg, 'set_visible'):
                self.connecting_bg.set_visible(visible)
            if hasattr(self.connecting_box, 'set_visible'):
                self.connecting_box.set_visible(visible)
        except Exception:
            pass
    
    def _connect_ssh(self):
        """Connect to SSH host"""
        if not self.connection:
            logger.error("No connection configured")
            return False
            
        # Ensure VTE terminal is properly initialized
        if not hasattr(self, 'vte') or self.vte is None:
            logger.error("VTE terminal not initialized")
            return False
        
        try:
            # Connect in a separate thread to avoid blocking UI
            thread = threading.Thread(target=self._connect_ssh_thread)
            thread.daemon = True
            thread.start()
            
            return True
            
        except Exception as e:
            logger.error(f"Failed to start SSH connection: {e}")
            GLib.idle_add(self._on_connection_failed, str(e))
            return False
    
    def _connect_ssh_thread(self):
        """SSH connection thread: directly spawn SSH and rely on its output for errors."""
        try:
            GLib.idle_add(self._setup_ssh_terminal)
        except Exception as e:
            logger.error(f"SSH connection failed: {e}")
            GLib.idle_add(self._on_connection_failed, str(e))
    
    def _setup_ssh_terminal(self):
        """Set up terminal with direct SSH command (called from main thread)"""
        try:
            # Build SSH command
            ssh_cmd = ['ssh']

            # Read SSH behavior from config with sane defaults
            try:
                ssh_cfg = self.config.get_ssh_config() if hasattr(self.config, 'get_ssh_config') else {}
            except Exception:
                ssh_cfg = {}
            apply_adv = bool(ssh_cfg.get('apply_advanced', False))
            connect_timeout = int(ssh_cfg.get('connection_timeout', 10)) if apply_adv else None
            connection_attempts = int(ssh_cfg.get('connection_attempts', 1)) if apply_adv else None
            keepalive_interval = int(ssh_cfg.get('keepalive_interval', 30)) if apply_adv else None
            keepalive_count = int(ssh_cfg.get('keepalive_count_max', 3)) if apply_adv else None
            strict_host = str(ssh_cfg.get('strict_host_key_checking', '')) if apply_adv else ''
            auto_add_host_keys = bool(ssh_cfg.get('auto_add_host_keys', True))
            batch_mode = bool(ssh_cfg.get('batch_mode', False)) if apply_adv else False
            compression = bool(ssh_cfg.get('compression', True)) if apply_adv else False

            # Determine auth method from connection
            password_auth_selected = False
            try:
                # In our UI: 0 = key-based, 1 = password
                password_auth_selected = (getattr(self.connection, 'auth_method', 0) == 1)
            except Exception:
                password_auth_selected = False

            # Apply advanced args only when user explicitly enabled them
            if apply_adv:
                # Only enable BatchMode when NOT doing password auth (BatchMode disables prompts)
                if batch_mode and not password_auth_selected:
                    ssh_cmd.extend(['-o', 'BatchMode=yes'])
                if connect_timeout is not None:
                    ssh_cmd.extend(['-o', f'ConnectTimeout={connect_timeout}'])
                if connection_attempts is not None:
                    ssh_cmd.extend(['-o', f'ConnectionAttempts={connection_attempts}'])
                if keepalive_interval is not None:
                    ssh_cmd.extend(['-o', f'ServerAliveInterval={keepalive_interval}'])
                if keepalive_count is not None:
                    ssh_cmd.extend(['-o', f'ServerAliveCountMax={keepalive_count}'])
                if strict_host:
                    ssh_cmd.extend(['-o', f'StrictHostKeyChecking={strict_host}'])
                if compression:
                    ssh_cmd.append('-C')

            # Apply auto-add host keys policy even when advanced block is off, unless user explicitly set a policy
            try:
                if (not strict_host) and auto_add_host_keys:
                    ssh_cmd.extend(['-o', 'StrictHostKeyChecking=accept-new'])
            except Exception:
                pass

            # Ensure SSH exits immediately on failure rather than waiting in background
            ssh_cmd.extend(['-o', 'ExitOnForwardFailure=yes'])
            
            # Default to accepting new host keys non-interactively on fresh installs
            try:
                if (not strict_host) and auto_add_host_keys:
                    ssh_cmd.extend(['-o', 'StrictHostKeyChecking=accept-new'])
            except Exception:
                pass
            
            # Only add verbose flag if explicitly enabled in config
            try:
                ssh_cfg = self.config.get_ssh_config() if hasattr(self.config, 'get_ssh_config') else {}
                verbosity = int(ssh_cfg.get('verbosity', 0))
                debug_enabled = bool(ssh_cfg.get('debug_enabled', False))
                v = max(0, min(3, verbosity))
                for _ in range(v):
                    ssh_cmd.append('-v')
                # Map verbosity to LogLevel to ensure messages are not suppressed by defaults
                if v == 1:
                    ssh_cmd.extend(['-o', 'LogLevel=VERBOSE'])
                elif v == 2:
                    ssh_cmd.extend(['-o', 'LogLevel=DEBUG2'])
                elif v >= 3:
                    ssh_cmd.extend(['-o', 'LogLevel=DEBUG3'])
                elif debug_enabled:
                    ssh_cmd.extend(['-o', 'LogLevel=DEBUG'])
                if v > 0 or debug_enabled:
                    logger.debug("SSH verbosity configured: -v x %d, LogLevel set", v)
            except Exception as e:
                logger.warning(f"Could not check SSH verbosity/debug settings: {e}")
                # Default to non-verbose on error
            
            # Add key file/options only for key-based auth
            if not password_auth_selected:
                if hasattr(self.connection, 'keyfile') and self.connection.keyfile and \
                   os.path.isfile(self.connection.keyfile) and \
                   not self.connection.keyfile.startswith('Select key file'):
                    ssh_cmd.extend(['-i', self.connection.keyfile])
                    logger.debug(f"Using SSH key: {self.connection.keyfile}")
                    # Enforce using only the specified key when key_select_mode == 1
                    try:
                        if int(getattr(self.connection, 'key_select_mode', 0) or 0) == 1:
                            ssh_cmd.extend(['-o', 'IdentitiesOnly=yes'])
                    except Exception:
                        pass
                else:
                    logger.debug("No valid SSH key specified, using default")
            else:
                # Prefer password/interactive methods when user chose password auth
                ssh_cmd.extend(['-o', 'PreferredAuthentications=password,keyboard-interactive'])
                ssh_cmd.extend(['-o', 'PubkeyAuthentication=no'])

                # If we have a stored password and sshpass is available, use it to avoid prompts
                password_value = None
                try:
                    password_value = getattr(self.connection, 'password', None)
                    if (not password_value) and hasattr(self, 'connection_manager') and self.connection_manager:
                        password_value = self.connection_manager.get_password(self.connection.host, self.connection.username)
                except Exception:
                    password_value = None

                if password_value and shutil.which('sshpass'):
                    # Prepend sshpass and move current ssh_cmd after it
                    ssh_cmd = ['sshpass', '-p', str(password_value), 'ssh'] + ssh_cmd[1:]
                    # Do not log plaintext password
                    try:
                        masked = [part if part != str(password_value) else '******' for part in ssh_cmd]
                        logger.debug("Using sshpass with command: %s", ' '.join(masked))
                    except Exception:
                        pass
            
            # Add X11 forwarding if enabled
            if hasattr(self.connection, 'x11_forwarding') and self.connection.x11_forwarding:
                ssh_cmd.append('-X')
            
            # Prepare command-related options (must appear before host)
            remote_cmd = ''
            local_cmd = ''
            try:
                if hasattr(self.connection, 'remote_command'):
                    remote_cmd = (self.connection.remote_command or '').strip()
                if not remote_cmd and hasattr(self.connection, 'data'):
                    remote_cmd = (self.connection.data.get('remote_command') or '').strip()
            except Exception:
                remote_cmd = ''
            try:
                if hasattr(self.connection, 'local_command'):
                    local_cmd = (self.connection.local_command or '').strip()
                if not local_cmd and hasattr(self.connection, 'data'):
                    local_cmd = (self.connection.data.get('local_command') or '').strip()
            except Exception:
                local_cmd = ''

            # If remote command is specified, request a TTY (twice for force allocation)
            if remote_cmd:
                ssh_cmd.extend(['-t', '-t'])

            # If local command specified, allow and set it via options
            if local_cmd:
                ssh_cmd.extend(['-o', 'PermitLocalCommand=yes'])
                # Pass exactly as user provided, letting ssh parse quoting
                ssh_cmd.extend(['-o', f'LocalCommand={local_cmd}'])

            # Add port forwarding rules
            if hasattr(self.connection, 'forwarding_rules'):
                for rule in self.connection.forwarding_rules:
                    if not rule.get('enabled', True):
                        continue
                        
                    rule_type = rule.get('type')
                    listen_addr = rule.get('listen_addr', '127.0.0.1')
                    listen_port = rule.get('listen_port')
                    
                    if rule_type == 'dynamic' and listen_port:
                        try:
                            ssh_cmd.extend(['-D', f"{listen_addr}:{listen_port}"])
                            logger.debug(f"Added dynamic port forwarding: {listen_addr}:{listen_port}")
                        except Exception as e:
                            logger.error(f"Failed to set up dynamic forwarding: {e}")
                            
                    elif rule_type == 'local' and listen_port and 'remote_host' in rule and 'remote_port' in rule:
                        try:
                            remote_host = rule.get('remote_host', 'localhost')
                            remote_port = rule.get('remote_port')
                            ssh_cmd.extend(['-L', f"{listen_addr}:{listen_port}:{remote_host}:{remote_port}"])
                            logger.debug(f"Added local port forwarding: {listen_addr}:{listen_port} -> {remote_host}:{remote_port}")
                        except Exception as e:
                            logger.error(f"Failed to set up local forwarding: {e}")
                            
                    # Handle remote port forwarding (remote bind -> local destination)
                    elif rule_type == 'remote' and listen_port:
                        try:
                            local_host = rule.get('local_host') or rule.get('remote_host', 'localhost')
                            local_port = rule.get('local_port') or rule.get('remote_port')
                            if local_port:
                                ssh_cmd.extend(['-R', f"{listen_addr}:{listen_port}:{local_host}:{local_port}"])
                                logger.debug(f"Added remote port forwarding: {listen_addr}:{listen_port} -> {local_host}:{local_port}")
                        except Exception as e:
                            logger.error(f"Failed to set up remote forwarding: {e}")
            
            # Add host and user
            ssh_cmd.append(f"{self.connection.username}@{self.connection.host}" if hasattr(self.connection, 'username') and self.connection.username else self.connection.host)

            # Add port if not default (ideally before host, but keep consistent with existing behavior)
            if hasattr(self.connection, 'port') and self.connection.port != 22:
                ssh_cmd.extend(['-p', str(self.connection.port)])

            # Append remote command last so ssh treats it as the command to run, ensure shell remains active
            if remote_cmd:
                final_remote_cmd = remote_cmd if 'exec $SHELL' in remote_cmd else f"{remote_cmd} ; exec $SHELL -l"
                # Append as single argument; let shell on remote parse quotes. Keep as-is to allow user quoting.
                ssh_cmd.append(final_remote_cmd)
            
            # Avoid logging password when sshpass is used
            try:
                if ssh_cmd[:2] == ['sshpass', '-p'] and len(ssh_cmd) > 2:
                    masked_cmd = ssh_cmd.copy()
                    masked_cmd[2] = '******'
                    logger.debug(f"SSH command: {' '.join(masked_cmd)}")
                else:
                    logger.debug(f"SSH command: {' '.join(ssh_cmd)}")
            except Exception:
                logger.debug("Prepared SSH command")
            
            # Create a new PTY for the terminal
            pty = Vte.Pty.new_sync(Vte.PtyFlags.DEFAULT)
            
            # Start the SSH process using VTE's spawn_async with our PTY
            self.vte.spawn_async(
                Vte.PtyFlags.DEFAULT,
                os.path.expanduser('~') or '/',
                ssh_cmd,
                None,  # Environment (use default)
                GLib.SpawnFlags.DEFAULT,
                None,  # Child setup function
                None,  # Child setup data
                -1,    # Timeout (-1 = default)
                None,  # Cancellable
                self._on_spawn_complete,
                ()     # User data - empty tuple for Flatpak VTE compatibility
            )
            
            # Store the PTY for later cleanup
            self.pty = pty
            try:
                import time
                self._spawn_start_time = time.time()
            except Exception:
                self._spawn_start_time = None
            
            # Defer marking as connected until spawn completes
            try:
                self.apply_theme()
            except Exception:
                pass
            
            # Apply theme after connection is established
            self.apply_theme()
            
            # Focus the terminal
            self.vte.grab_focus()
            
            logger.info(f"SSH terminal connected to {self.connection}")
            
        except Exception as e:
            logger.error(f"Failed to setup SSH terminal: {e}")
            self._on_connection_failed(str(e))
    
    def _on_spawn_complete(self, terminal, pid, error, user_data=None):
        """Called when terminal spawn is complete"""
        if error:
            logger.error(f"Terminal spawn failed: {error}")
            # Ensure theme is applied before showing error so bg doesn't flash white
            try:
                self.apply_theme()
            except Exception:
                pass
            self._on_connection_failed(str(error))
            return
            
        logger.debug(f"Terminal spawned with PID: {pid}")
        self.process_pid = pid
        
        try:
            # Get and store process group ID
            self.process_pgid = os.getpgid(pid)
            logger.debug(f"Process group ID: {self.process_pgid}")
            
            # Store process info for cleanup
            with process_manager.lock:
                process_manager.processes[pid] = {
                    'terminal': weakref.ref(self),
                    'start_time': datetime.now(),
                    'command': 'ssh',
                    'pgid': self.process_pgid
                }
            
            # Grab focus and apply theme
            self.vte.grab_focus()
            self.apply_theme()

            # Spawn succeeded; mark as connected and hide overlay
            self.is_connected = True
            self.emit('connection-established')
            self._set_connecting_overlay_visible(False)
            # Ensure any reconnect/disconnected banner is hidden upon successful spawn
            try:
                self._set_disconnected_banner_visible(False)
            except Exception:
                pass
            
        except Exception as e:
            logger.error(f"Error in spawn complete: {e}")
            self._on_connection_failed(str(e))
    
    def _on_connection_failed(self, error_message):
        """Handle connection failure (called from main thread)"""
        logger.error(f"Connection failed: {error_message}")
        
        # Ensure theme is applied so background remains consistent
        try:
            self.apply_theme()
        except Exception:
            pass
        
        # Show error in terminal
        try:
            self.vte.feed(f"\r\n\x1b[31mConnection failed: {error_message}\x1b[0m\r\n".encode('utf-8'))
        except Exception as e:
            logger.error(f"Error displaying connection error: {e}")
        
        self.is_connected = False
        self.emit('connection-failed', error_message)

    

    def _show_forwarding_error_dialog(self, message):
        try:
            dialog = Adw.MessageDialog(
                transient_for=self.get_root() if hasattr(self, 'get_root') else None,
                modal=True,
                heading="Port Forwarding Error",
                body=str(message)
            )
            dialog.add_response('ok', 'OK')
            dialog.set_default_response('ok')
            dialog.present()
        except Exception as e:
            logger.debug(f"Failed to present forwarding error dialog: {e}")
        return False
        
    def apply_theme(self, theme_name=None):
        """Apply terminal theme and font settings
        
        Args:
            theme_name (str, optional): Name of the theme to apply. If None, uses the saved theme.
        """
        try:
            if theme_name is None and self.config:
                # Get the saved theme from config
                theme_name = self.config.get_setting('terminal.theme', 'default')
                
            # Get the theme profile from config
            if self.config:
                profile = self.config.get_terminal_profile(theme_name)
            else:
                # Fallback default theme
                profile = {
                    'foreground': '#000000',  # Black text
                    'background': '#FFFFFF',  # White background
                    'font': 'Monospace 12',
                    'cursor_color': '#000000',
                    'highlight_background': '#4A90E2',
                    'highlight_foreground': '#FFFFFF',
                }
            
            # Set colors
            fg_color = Gdk.RGBA()
            fg_color.parse(profile['foreground'])
            
            bg_color = Gdk.RGBA()
            bg_color.parse(profile['background'])
            
            cursor_color = Gdk.RGBA()
            cursor_color.parse(profile.get('cursor_color', profile['foreground']))
            
            highlight_bg = Gdk.RGBA()
            highlight_bg.parse(profile.get('highlight_background', '#4A90E2'))
            
            highlight_fg = Gdk.RGBA()
            highlight_fg.parse(profile.get('highlight_foreground', profile['foreground']))
            
            # Apply colors to terminal
            self.vte.set_colors(fg_color, bg_color, None)
            self.vte.set_color_cursor(cursor_color)
            self.vte.set_color_highlight(highlight_bg)
            self.vte.set_color_highlight_foreground(highlight_fg)

            # Also color the container background to prevent white flash before VTE paints
            try:
                rgba = bg_color
                # For Gtk4, setting the widget style via CSS provider
                provider = Gtk.CssProvider()
                css = f".terminal-bg {{ background-color: rgba({int(rgba.red*255)}, {int(rgba.green*255)}, {int(rgba.blue*255)}, {rgba.alpha}); }}"
                provider.load_from_data(css.encode('utf-8'))
                display = Gdk.Display.get_default()
                if display:
                    Gtk.StyleContext.add_provider_for_display(display, provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
                if hasattr(self, 'add_css_class'):
                    self.add_css_class('terminal-bg')
                if hasattr(self.scrolled_window, 'add_css_class'):
                    self.scrolled_window.add_css_class('terminal-bg')
                if hasattr(self.vte, 'add_css_class'):
                    self.vte.add_css_class('terminal-bg')
            except Exception as e:
                logger.debug(f"Failed to set container background: {e}")
            
            # Set font
            font_desc = Pango.FontDescription.from_string(profile['font'])
            self.vte.set_font(font_desc)
            
            # Force a redraw
            self.vte.queue_draw()
            
            logger.debug(f"Applied terminal theme: {theme_name or 'default'}")
            
        except Exception as e:
            logger.error(f"Failed to apply terminal theme: {e}")
            
    def force_style_refresh(self):
        """Force a style refresh of the terminal widget."""
        self.apply_theme()
    
    def setup_terminal(self):
        """Initialize the VTE terminal with appropriate settings."""
        logger.info("Setting up terminal...")
        
        try:
            # Set terminal font
            font_desc = Pango.FontDescription()
            font_desc.set_family("Monospace")
            font_desc.set_size(12 * Pango.SCALE)  # Slightly larger default font
            self.vte.set_font(font_desc)
            
            # Do not force a light default; theme will define colors
            self.apply_theme()
            
            # Set cursor properties
            self.vte.set_cursor_blink_mode(Vte.CursorBlinkMode.ON)
            self.vte.set_cursor_shape(Vte.CursorShape.BLOCK)
            
            # Set scrollback lines
            self.vte.set_scrollback_lines(10000)
            
            # Set word char exceptions (for double-click selection)
            try:
                # Try the newer API first (VTE 0.60+)
                if hasattr(self.vte, 'set_word_char_exceptions'):
                    self.vte.set_word_char_exceptions("@-./_~")
                    logger.debug("Set word char exceptions using VTE 0.60+ API")
                # Fall back to the older API if needed
                elif hasattr(self.vte, 'set_word_char_options'):
                    self.vte.set_word_char_options("@-./_~")
                    logger.debug("Set word char exceptions using older VTE API")
            except Exception as e:
                logger.warning(f"Could not set word char options: {e}")
            
            # Set cursor and selection colors
            try:
                cursor_color = Gdk.RGBA()
                cursor_color.parse('black')  # Black cursor
                
                if hasattr(self.vte, 'set_color_cursor'):
                    self.vte.set_color_cursor(cursor_color)
                    logger.debug("Set cursor color")
                
                # Set selection colors
                if hasattr(self.vte, 'set_color_highlight'):
                    highlight_bg = Gdk.RGBA()
                    highlight_bg.parse('#4A90E2')  # Light blue highlight
                    self.vte.set_color_highlight(highlight_bg)
                    logger.debug("Set highlight color")
                    
                    highlight_fg = Gdk.RGBA()
                    highlight_fg.parse('white')
                    if hasattr(self.vte, 'set_color_highlight_foreground'):
                        self.vte.set_color_highlight_foreground(highlight_fg)
                        logger.debug("Set highlight foreground color")
                        
            except Exception as e:
                logger.warning(f"Could not set terminal colors: {e}")
            
            # Enable mouse reporting if available
            if hasattr(self.vte, 'set_mouse_autohide'):
                self.vte.set_mouse_autohide(True)
                logger.debug("Enabled mouse autohide")
                
            # Set terminal encoding
            try:
                self.vte.set_encoding('UTF-8')
                logger.debug("Set terminal encoding to UTF-8")
            except Exception as e:
                logger.warning(f"Could not set terminal encoding: {e}")
                
            # Enable bold text
            try:
                if hasattr(self.vte, 'set_allow_bold'):
                    self.vte.set_allow_bold(True)
                    logger.debug("Enabled bold text")
            except Exception as e:
                logger.warning(f"Could not enable bold text: {e}")
                
            logger.info("Terminal setup complete")
            
            # Enable bold text
            self.vte.set_allow_bold(True)
            
            # Show the terminal
            self.vte.show()
            logger.info("Terminal setup complete")
            
        except Exception as e:
            logger.error(f"Error in setup_terminal: {e}", exc_info=True)
            raise
        
        # Install terminal shortcuts and custom context menu
        self._install_shortcuts()
        self._setup_context_menu()

    def _setup_context_menu(self):
        """Set up a robust per-terminal context menu and actions."""
        try:
            # Per-widget action group
            self._menu_actions = Gio.SimpleActionGroup()
            act_copy = Gio.SimpleAction.new("copy", None)
            act_copy.connect("activate", lambda a, p: self.copy_text())
            self._menu_actions.add_action(act_copy)
            act_paste = Gio.SimpleAction.new("paste", None)
            act_paste.connect("activate", lambda a, p: self.paste_text())
            self._menu_actions.add_action(act_paste)
            act_selall = Gio.SimpleAction.new("select_all", None)
            act_selall.connect("activate", lambda a, p: self.select_all())
            self._menu_actions.add_action(act_selall)
            self.insert_action_group('term', self._menu_actions)

            # Menu model
            self._menu_model = Gio.Menu()
            self._menu_model.append(_("Copy"), "term.copy")
            self._menu_model.append(_("Paste"), "term.paste")
            self._menu_model.append(_("Select All"), "term.select_all")

            # Popover
            self._menu_popover = Gtk.PopoverMenu.new_from_model(self._menu_model)
            self._menu_popover.set_has_arrow(True)
            self._menu_popover.set_parent(self.vte)

            # Right-click gesture to open popover
            gesture = Gtk.GestureClick()
            gesture.set_button(0)
            def _on_pressed(gest, n_press, x, y):
                try:
                    btn = 0
                    try:
                        btn = gest.get_current_button()
                    except Exception:
                        pass
                    if btn not in (Gdk.BUTTON_SECONDARY, 3):
                        return
                    # Focus terminal first for reliable copy/paste
                    try:
                        self.vte.grab_focus()
                    except Exception:
                        pass
                    # Position popover near click
                    try:
                        rect = Gdk.Rectangle()
                        rect.x = int(x)
                        rect.y = int(y)
                        rect.width = 1
                        rect.height = 1
                        self._menu_popover.set_pointing_to(rect)
                    except Exception:
                        pass
                    self._menu_popover.popup()
                except Exception:
                    pass
            gesture.connect('pressed', _on_pressed)
            self.vte.add_controller(gesture)
        except Exception as e:
            logger.debug(f"Context menu setup skipped/failed: {e}")

    def _install_shortcuts(self):
        """Install local shortcuts on the VTE widget for copy/paste/select-all"""
        try:
            controller = Gtk.ShortcutController()
            controller.set_scope(Gtk.ShortcutScope.LOCAL)
            
            def _cb_copy(widget, *args):
                try:
                    self.copy_text()
                except Exception:
                    pass
                return True
            def _cb_paste(widget, *args):
                try:
                    self.paste_text()
                except Exception:
                    pass
                return True
            def _cb_select_all(widget, *args):
                try:
                    self.select_all()
                except Exception:
                    pass
                return True
            
            controller.add_shortcut(Gtk.Shortcut.new(
                Gtk.ShortcutTrigger.parse_string("<Primary><Shift>c"),
                Gtk.CallbackAction.new(_cb_copy)
            ))
            controller.add_shortcut(Gtk.Shortcut.new(
                Gtk.ShortcutTrigger.parse_string("<Primary><Shift>v"),
                Gtk.CallbackAction.new(_cb_paste)
            ))
            controller.add_shortcut(Gtk.Shortcut.new(
                Gtk.ShortcutTrigger.parse_string("<Primary><Shift>a"),
                Gtk.CallbackAction.new(_cb_select_all)
            ))
            
            self.vte.add_controller(controller)
        except Exception as e:
            logger.debug(f"Failed to install shortcuts: {e}")
            
    # PTY forwarding is now handled automatically by VTE
    # No need for manual PTY management in this implementation
    
    def reconnect(self):
        """Reconnect the terminal with updated connection settings"""
        logger.info("Reconnecting terminal with updated settings...")
        was_connected = self.is_connected
        
        # Disconnect if currently connected
        if was_connected:
            self.disconnect()
        
        # Reconnect after a short delay to allow disconnection to complete
        def _reconnect():
            if self._connect_ssh():
                logger.info("Terminal reconnected with updated settings")
                # Ensure theme is applied after reconnection
                self.apply_theme()
                return True
            else:
                logger.error("Failed to reconnect terminal with updated settings")
                return False
        
        GLib.timeout_add(500, _reconnect)  # 500ms delay before reconnecting
    
    def _on_connection_updated_signal(self, sender, connection):
        """Signal handler for connection-updated signal"""
        self._on_connection_updated(connection)
        
    def _on_connection_updated(self, connection):
        """Called when connection settings are updated
        
        Note: We don't automatically reconnect here to prevent infinite loops.
        The main window will handle the reconnection flow after user confirmation.
        """
        if connection == self.connection:
            logger.info("Connection settings updated, waiting for user confirmation to reconnect...")
            # Just update our connection reference, don't reconnect automatically
            self.connection = connection
    
    def _on_connection_established(self):
        """Handle successful SSH connection"""
        logger.info(f"SSH connection to {self.connection.host} established")
        self.is_connected = True
        
        # Update connection status in the connection manager
        self.connection.is_connected = True
        self.connection_manager.emit('connection-status-changed', self.connection, True)
        
        self.emit('connection-established')
        
        # Apply theme after connection is established
        self.apply_theme()
        # Hide any reconnect banner on success
        self._set_disconnected_banner_visible(False)
        
    def _on_connection_lost(self):
        """Handle SSH connection loss"""
        if self.is_connected:
            logger.info(f"SSH connection to {self.connection.host} lost")
            self.is_connected = False
            
            # Update connection status in the connection manager
            if hasattr(self, 'connection') and self.connection:
                self.connection.is_connected = False
                self.connection_manager.emit('connection-status-changed', self.connection, False)
            
            self.emit('connection-lost')
            # Show reconnect UI
            self._set_connecting_overlay_visible(False)
            self._set_disconnected_banner_visible(True, _('Connection lost.'))
    
    def on_child_exited(self, widget, status):
        """Called when the child process exits"""
        # On clean exit, close the tab without confirmation
        try:
            if hasattr(self, 'connection_manager') and hasattr(self, 'get_root'):
                root = self.get_root()
                # 0 or success-like statuses indicate user exit
                if status == 0 and root and hasattr(root, 'tab_view'):
                    page = root.tab_view.get_page(self)
                    if page:
                        # Suppress confirmation dialogs during programmatic close
                        try:
                            setattr(root, '_suppress_close_confirmation', True)
                            root.tab_view.close_page(page)
                        finally:
                            try:
                                setattr(root, '_suppress_close_confirmation', False)
                            except Exception:
                                pass
                        return
        except Exception:
            pass
        if self.is_connected:
            self.is_connected = False
            logger.info(f"SSH session ended with status {status}")
            self.emit('connection-lost')
            # Show reconnect UI
            self._set_connecting_overlay_visible(False)
            self._set_disconnected_banner_visible(True, _('Session ended.'))
        else:
            # Early exit without connection; if forwarding was requested, show dialog
            try:
                if getattr(self.connection, 'forwarding_rules', None):
                    import time
                    fast_fail = (getattr(self, '_spawn_start_time', None) and (time.time() - self._spawn_start_time) < 5)
                    if fast_fail:
                        self._show_forwarding_error_dialog("SSH failed to start with requested port forwarding. Check if ports are available.")
                        # Also print red hint
                        try:
                            self.vte.feed(b"\r\n\x1b[31mPort forwarding failed.\x1b[0m\r\n")
                        except Exception:
                            pass
            except Exception as e:
                logger.debug(f"Error handling early child exit: {e}")
    
    def _on_terminal_input(self, widget, text, size):
        """Handle input from the terminal (handled automatically by VTE)"""
        pass
            
    def _on_terminal_resize(self, widget, width, height):
        """Handle terminal resize events"""
        # Update the SSH session if it exists
        if self.ssh_session and hasattr(self.ssh_session, 'change_terminal_size'):
            asyncio.create_task(
                self.ssh_session.change_terminal_size(
                    height, width, 0, 0
                )
            )
    
    def _on_ssh_disconnected(self, exc):
        """Called when SSH connection is lost"""
        if self.is_connected:
            self.is_connected = False
            if exc:
                logger.error(f"SSH connection lost: {exc}")
            GLib.idle_add(lambda: self.emit('connection-lost'))
    
    def _setup_process_group(self, spawn_data):
        """Setup function called after fork but before exec"""
        # Create new process group for the child process
        os.setpgrp()
        
    def _get_terminal_pid(self):
        """Get the PID of the terminal's child process"""
        # First try the stored PID
        if self.process_pid:
            try:
                # Verify the process still exists
                os.kill(self.process_pid, 0)
                return self.process_pid
            except (ProcessLookupError, OSError):
                pass
        
        # Fall back to getting from PTY or VTE helpers
        try:
            # Prefer PID recorded at spawn complete
            if getattr(self, 'process_pid', None):
                return self.process_pid
            pty = self.vte.get_pty()
            if pty and hasattr(pty, 'get_pid'):
                pid = pty.get_pid()
                if pid:
                    self.process_pid = pid
                    return pid
        except Exception as e:
            logger.error(f"Error getting terminal PID: {e}")
        
        return None
        
    def _on_destroy(self, widget):
        """Handle widget destruction"""
        logger.debug(f"Terminal widget {self.session_id} being destroyed")
        
        # Disconnect from connection manager signals
        if hasattr(self, '_connection_updated_handler') and hasattr(self.connection_manager, 'disconnect'):
            try:
                self.connection_manager.disconnect(self._connection_updated_handler)
                logger.debug("Disconnected from connection manager signals")
            except Exception as e:
                logger.error(f"Error disconnecting from connection manager: {e}")
        
        # Disconnect the terminal
        self.disconnect()

    def _terminate_process_tree(self, pid):
        """Terminate a process and all its children"""
        try:
            # First try to get the process group
            try:
                pgid = os.getpgid(pid)
                logger.debug(f"Terminating process group {pgid}")
                os.killpg(pgid, signal.SIGTERM)
                
                # Give processes a moment to shut down
                time.sleep(0.5)
                
                # Check if any processes are still running
                try:
                    os.killpg(pgid, 0)  # Check if process group exists
                    logger.debug(f"Process group {pgid} still running, sending SIGKILL")
                    os.killpg(pgid, signal.SIGKILL)
                except (ProcessLookupError, OSError):
                    pass  # Process group already gone
                    
            except ProcessLookupError:
                logger.debug(f"Process {pid} already terminated")
                return
                
            # Wait for process to terminate
            try:
                os.waitpid(pid, os.WNOHANG)
            except (ChildProcessError, OSError):
                pass
                
        except Exception as e:
            logger.error(f"Error terminating process {pid}: {e}")
    
    def _cleanup_process(self, pid):
        """Clean up a process by PID"""
        if not pid:
            return False
            
        try:
            # Try to get process info from manager first
            pgid = None
            with process_manager.lock:
                if pid in process_manager.processes:
                    pgid = process_manager.processes[pid].get('pgid')
            
            # Fall back to getting PGID from system
            if not pgid:
                try:
                    pgid = os.getpgid(pid)
                except ProcessLookupError:
                    logger.debug(f"Process {pid} already terminated")
                    return True
            
            # First try a clean termination
            try:
                os.kill(pid, signal.SIGTERM)
                logger.debug(f"Sent SIGTERM to process {pid} (PGID: {pgid})")
                
                # Wait for clean termination
                for _ in range(5):  # Wait up to 0.5 seconds
                    try:
                        os.kill(pid, 0)
                        time.sleep(0.1)
                    except ProcessLookupError:
                        logger.debug(f"Process {pid} terminated cleanly")
                        return True
                
                # If still running, force kill
                try:
                    os.kill(pid, 0)  # Check if still exists
                    logger.debug(f"Process {pid} still running, sending SIGKILL")
                    if pgid:
                        try:
                            os.killpg(pgid, signal.SIGKILL)
                        except ProcessLookupError:
                            pass
                    os.kill(pid, signal.SIGKILL)
                    return True
                except ProcessLookupError:
                    return True
                    
            except ProcessLookupError:
                return True
                
        except Exception as e:
            logger.error(f"Error terminating process {pid}: {e}")
            return False
    
    def disconnect(self):
        """Close the SSH connection and clean up resources"""
        if not self.is_connected:
            return
            
        logger.debug(f"Disconnecting SSH session {self.session_id}...")
        was_connected = self.is_connected
        self.is_connected = False
        
        # Update connection status in the connection manager if we were connected
        if was_connected and hasattr(self, 'connection') and self.connection:
            self.connection.is_connected = False
            if hasattr(self, 'connection_manager') and self.connection_manager:
                GLib.idle_add(self.connection_manager.emit, 'connection-status-changed', self.connection, False)
        
        try:
            # Try to get the terminal's child PID
            pid = self._get_terminal_pid()
            
            # Collect all PIDs that need to be cleaned up
            pids_to_clean = set()
            
            # Add the main process PID if available
            if pid:
                pids_to_clean.add(pid)
            
            # Add the process group ID if available
            if hasattr(self, 'process_pgid') and self.process_pgid:
                pids_to_clean.add(self.process_pgid)
            
            # Add any PIDs from the process manager
            with process_manager.lock:
                for proc_pid, proc_info in list(process_manager.processes.items()):
                    if proc_info.get('terminal')() is self:
                        pids_to_clean.add(proc_pid)
                        if 'pgid' in proc_info:
                            pids_to_clean.add(proc_info['pgid'])
            
            # Clean up all collected PIDs
            for cleanup_pid in pids_to_clean:
                if cleanup_pid:
                    self._cleanup_process(cleanup_pid)
            
            # Clean up PTY if it exists
            if hasattr(self, 'pty') and self.pty:
                try:
                    self.pty.close()
                except Exception as e:
                    logger.error(f"Error closing PTY: {e}")
                finally:
                    self.pty = None
            
            # Clean up from process manager
            with process_manager.lock:
                for proc_pid in list(process_manager.processes.keys()):
                    proc_info = process_manager.processes[proc_pid]
                    if proc_info.get('terminal')() is self:
                        del process_manager.processes[proc_pid]
            
            # Do not hard-reset here; keep current theme/colors
            
            logger.debug(f"Cleaned up {len(pids_to_clean)} processes for session {self.session_id}")
            
        except Exception as e:
            logger.error(f"Error during disconnect: {e}")
        finally:
            # Clean up references
            self.process_pid = None
            self.process_pgid = None
            
            # Ensure we always emit the connection-lost signal
            self.emit('connection-lost')
            logger.debug(f"SSH session {self.session_id} disconnected")
    
    def _on_connection_failed(self, error_message):
        """Handle connection failure (called from main thread)"""
        logger.error(f"Connection failed: {error_message}")
        
        try:
            # Show error in terminal
            error_msg = f"\r\n\x1b[31mConnection failed: {error_message}\x1b[0m\r\n"
            self.vte.feed(error_msg.encode('utf-8'))

            self.is_connected = False

            # Clean up PTY if it exists
            if hasattr(self, 'pty') and self.pty:
                self.pty.close()
                del self.pty

            # Do not reset here to avoid losing theme; leave buffer with error text

            # Notify UI
            self.emit('connection-failed', error_message)

            # Show reconnect banner for new-connection failures as well
            self._set_connecting_overlay_visible(False)
            # Detect timeout-ish messages to provide clearer text
            msg_lower = (error_message or '').lower()
            if 'timeout' in msg_lower or 'timed out' in msg_lower:
                banner_text = _('Connection timeout. Try again?')
            elif 'failed to read ssh banner' in msg_lower:
                banner_text = _('Server not ready yet. Try again?')
            else:
                banner_text = _('Connection failed.')
            self._set_disconnected_banner_visible(True, banner_text)

        except Exception as e:
            logger.error(f"Error in _on_connection_failed: {e}")

    def on_child_exited(self, terminal, status):
        """Handle terminal child process exit"""
        logger.debug(f"Terminal child exited with status: {status}")

        # Clean up process tracking immediately since the process has already exited
        try:
            pid = self._get_terminal_pid()
            if pid:
                with process_manager.lock:
                    if pid in process_manager.processes:
                        logger.debug(f"Removing exited process {pid} from tracking")
                        del process_manager.processes[pid]
            
            # Also remove this terminal from the process manager's terminal tracking
            with process_manager.lock:
                if self in process_manager.terminals:
                    logger.debug(f"Removing exited terminal {id(self)} from tracking")
                    process_manager.terminals.remove(self)
            
            # Clear process PID to prevent further cleanup attempts
            if hasattr(self, 'process_pid'):
                self.process_pid = None
        except Exception as e:
            logger.debug(f"Error cleaning up exited process tracking: {e}")

        # Normalize exit status: GLib may pass waitpid-style status
        exit_code = None
        try:
            if os.WIFEXITED(status):
                exit_code = os.WEXITSTATUS(status)
            else:
                # If not a normal exit or os.WIF* not applicable, best-effort mapping
                exit_code = status if 0 <= int(status) < 256 else ((int(status) >> 8) & 0xFF)
        except Exception:
            try:
                exit_code = int(status)
            except Exception:
                exit_code = status

        # If user explicitly typed 'exit' (clean status 0), close tab immediately
        try:
            if exit_code == 0 and hasattr(self, 'get_root'):
                root = self.get_root()
                if root and hasattr(root, 'tab_view'):
                    page = root.tab_view.get_page(self)
                    if page:
                        try:
                            setattr(root, '_suppress_close_confirmation', True)
                            root.tab_view.close_page(page)
                        finally:
                            try:
                                setattr(root, '_suppress_close_confirmation', False)
                            except Exception:
                                pass
                        return
        except Exception:
            pass

        # Non-zero or unknown exit: treat as connection lost and show banner
        if self.connection:
            self.connection.is_connected = False

        self.disconnect()
        self.emit('connection-lost')
        # Show reconnect UI
        self._set_connecting_overlay_visible(False)
        self._set_disconnected_banner_visible(True, _('Session ended.'))

    def on_title_changed(self, terminal):
        """Handle terminal title change"""
        title = terminal.get_window_title()
        if title:
            self.emit('title-changed', title)
        # If terminal is connected and a title update occurs (often when prompt is ready),
        # ensure the reconnect banner is hidden
        try:
            if getattr(self, 'is_connected', False):
                self._set_disconnected_banner_visible(False)
        except Exception:
            pass

    def on_bell(self, terminal):
        """Handle terminal bell"""
        # Could implement visual bell or notification here
        pass

    def copy_text(self):
        """Copy selected text to clipboard"""
        if self.vte.get_has_selection():
            self.vte.copy_clipboard_format(Vte.Format.TEXT)

    def paste_text(self):
        """Paste text from clipboard"""
        self.vte.paste_clipboard()

    def select_all(self):
        """Select all text in terminal"""
        self.vte.select_all()

    def reset_terminal(self):
        """Reset terminal"""
        self.vte.reset(True, True)

    def reset_and_clear(self):
        """Reset and clear terminal"""
        self.vte.reset(True, False)

    def search_text(self, text, case_sensitive=False, regex=False):
        """Search for text in terminal"""
        try:
            # Create search regex
            if regex:
                search_regex = GLib.Regex.new(text, 0 if case_sensitive else GLib.RegexCompileFlags.CASELESS, 0)
            else:
                escaped_text = GLib.regex_escape_string(text)
                search_regex = GLib.Regex.new(escaped_text, 0 if case_sensitive else GLib.RegexCompileFlags.CASELESS, 0)
            
            # Set search regex
            self.vte.search_set_regex(search_regex, 0)
            
            # Find next match
            return self.vte.search_find_next()
            
        except Exception as e:
            logger.error(f"Search failed: {e}")
            return False

    def get_connection_info(self):
        """Get connection information"""
        if self.connection:
            return {
                'nickname': self.connection.nickname,
                'host': self.connection.host,
                'username': self.connection.username,
                'connected': self.is_connected
            }
        return None