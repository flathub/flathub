"""
Port forwarding dialog and configuration
"""

from dataclasses import dataclass
import gi
gi.require_version('Gtk', '4.0')
gi.require_version('Adw', '1')
from gi.repository import Gtk, Adw

@dataclass
class PortForward:
    """Port forwarding configuration in standard SSH config format"""
    type: str  # 'local' or 'remote'
    port: str  # Format: "[local_host:]local_port:remote_host:remote_port"
    bind_address: str = ''  # Bind address for RemoteForward
    
    @staticmethod
    def parse(forward_str: str, is_remote: bool = False) -> 'PortForward':
        """Parse a LocalForward or RemoteForward string from ssh_config format"""
        # Format: [bind_address:]port:host:hostport
        parts = forward_str.split(':')
        if len(parts) == 3:
            # port:host:hostport format
            local_port, remote_host, remote_port = parts
            bind_address = ''
            port = f"{local_port}:{remote_host}:{remote_port}"
        elif len(parts) == 4:
            # bind_address:port:host:hostport format
            bind_address, local_port, remote_host, remote_port = parts
            port = f"{local_port}:{remote_host}:{remote_port}"
        else:
            raise ValueError(f"Invalid port forward format: {forward_str}")
        
        return PortForward(
            type='remote' if is_remote else 'local',
            port=port,
            bind_address=bind_address
        )
    
    @property
    def as_tuple(self) -> tuple:
        """Get forward as (bind_address, port, remote_host, remote_port)"""
        parts = self.port.split(':')
        if len(parts) == 3:
            local_port, remote_host, remote_port = parts
            return (self.bind_address, int(local_port), remote_host, int(remote_port))
        return None
    
    def __str__(self) -> str:
        """Convert to ssh_config format string"""
        if self.bind_address:
            return f"{self.bind_address}:{self.port}"
        return self.port

class PortForwardDialog(Adw.Window):
    """Dialog for configuring a port forward"""
    
    def __init__(self, parent, config=None):
        super().__init__(
            title='Add Port Forward',
            transient_for=parent,
            modal=True,
            destroy_with_parent=True
        )
        
        self.header = Adw.HeaderBar()
        
        # Add cancel button
        cancel_button = Gtk.Button(label='Cancel')
        cancel_button.add_css_class('flat')
        cancel_button.connect('clicked', lambda btn: self.close())
        self.header.pack_start(cancel_button)
        
        # Add save button
        save_button = Gtk.Button(label='Add')
        save_button.add_css_class('suggested-action')
        save_button.connect('clicked', self.on_save_clicked)
        self.header.pack_end(save_button)
        
        # Main content
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        box.append(self.header)
        
        content = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        content.set_margin_start(24)
        content.set_margin_end(24)
        content.set_margin_top(24)
        content.set_margin_bottom(24)
        box.append(content)
        
        # Port forward type
        type_group = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        type_label = Gtk.Label(label='Type')
        type_label.set_halign(Gtk.Align.START)
        type_group.append(type_label)
        
        self.type_combo = Gtk.ComboBoxText()
        self.type_combo.append('local', 'Local (Outbound)')
        self.type_combo.append('remote', 'Remote (Inbound)')
        self.type_combo.set_active_id('local')
        type_group.append(self.type_combo)
        content.append(type_group)
        
        # Local settings
        local_group = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        local_group.set_margin_top(12)
        local_label = Gtk.Label(label='Local Settings')
        local_label.set_halign(Gtk.Align.START)
        local_group.append(local_label)
        
        local_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        
        self.local_host = Gtk.Entry()
        self.local_host.set_placeholder_text('localhost')
        self.local_host.set_hexpand(True)
        local_box.append(self.local_host)
        
        self.local_port = Gtk.SpinButton.new_with_range(1, 65535, 1)
        self.local_port.set_value(8000)
        local_box.append(self.local_port)
        
        local_group.append(local_box)
        content.append(local_group)
        
        # Remote settings
        remote_group = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        remote_group.set_margin_top(12)
        remote_label = Gtk.Label(label='Remote Settings')
        remote_label.set_halign(Gtk.Align.START)
        remote_group.append(remote_label)
        
        remote_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        
        self.remote_host = Gtk.Entry()
        self.remote_host.set_placeholder_text('localhost')
        self.remote_host.set_hexpand(True)
        remote_box.append(self.remote_host)
        
        self.remote_port = Gtk.SpinButton.new_with_range(1, 65535, 1)
        self.remote_port.set_value(8000)
        remote_box.append(self.remote_port)
        
        remote_group.append(remote_box)
        content.append(remote_group)
        
        self.set_content(box)

    def on_save_clicked(self, button):
        """Handle save button click"""
        # Convert inputs to forward config
        local_host = self.local_host.get_text() or 'localhost'
        remote_host = self.remote_host.get_text() or 'localhost'
        
        forward = PortForward(
            type=self.type_combo.get_active_id(),
            local_host=local_host,
            local_port=self.local_port.get_value_as_int(),
            remote_host=remote_host,
            remote_port=self.remote_port.get_value_as_int()
        )
        
        # Store result and close
        self.forward = forward
        self.response(Gtk.ResponseType.OK)
        
    def get_forward(self) -> PortForward:
        """Get the configured forward"""
        return getattr(self, 'forward', None)
