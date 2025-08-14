"""
Port Forwarding UI Components
Handles the UI for managing port forwarding rules
"""

import logging
from typing import Dict, List, Optional, Callable, Any

import gi
from gi.repository import Gtk, Adw, Gio, GLib, GObject

logger = logging.getLogger(__name__)

class PortForwardingRuleRow(Gtk.ListBoxRow):
    """
    Row widget for displaying a port forwarding rule in the rules list.
    
    This widget displays a single port forwarding rule with its type, listen address/port,
    and destination information (for local/remote forwarding) or SOCKS proxy info (for dynamic).
    """
    __gtype_name__ = 'PortForwardingRuleRow'
    
    def __init__(self, rule: Dict[str, Any], **kwargs):
        """
        Initialize a new PortForwardingRuleRow.
        
        Args:
            rule: The rule dictionary containing the rule data
            **kwargs: Additional arguments to pass to the parent class
        """
        super().__init__(**kwargs)
        self.rule = rule.copy()  # Store a copy to avoid modifying the original
        self.set_margin_top(4)
        self.set_margin_bottom(4)
        self.set_margin_start(4)
        self.set_margin_end(4)
        self.setup_ui()
        self.update_ui()
    
    def setup_ui(self):
        """Set up the UI components for the row"""
        # Create main container
        box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        box.set_margin_start(12)
        box.set_margin_end(12)
        box.set_margin_top(6)
        box.set_margin_bottom(6)
        
        # Add icon
        icon = Gtk.Image.new_from_icon_name("network-transmit-receive-symbolic")
        icon.set_valign(Gtk.Align.CENTER)
        box.append(icon)
        
        # Create a box to hold the type and details
        text_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=4)
        text_box.set_hexpand(True)
        
        # Type label (bold)
        self.type_label = Gtk.Label()
        self.type_label.set_halign(Gtk.Align.START)
        self.type_label.set_use_markup(True)
        
        # Details label
        self.details_label = Gtk.Label()
        self.details_label.set_halign(Gtk.Align.START)
        self.details_label.set_ellipsize(Pango.EllipsizeMode.END)
        
        # Add labels to the text box
        text_box.append(self.type_label)
        text_box.append(self.details_label)
        
        # Add the text box to the main container
        box.append(text_box)
        
        # Add action buttons
        button_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        
        # Edit button
        edit_button = Gtk.Button()
        edit_button.set_icon_name("document-edit-symbolic")
        edit_button.set_valign(Gtk.Align.CENTER)
        edit_button.add_css_class("flat")
        edit_button.connect("clicked", self.on_edit_clicked)
        
        # Delete button
        delete_button = Gtk.Button()
        delete_button.set_icon_name("user-trash-symbolic")
        delete_button.set_valign(Gtk.Align.CENTER)
        delete_button.add_css_class("flat")
        delete_button.add_css_class("error")
        delete_button.connect("clicked", self.on_delete_clicked)
        
        button_box.append(edit_button)
        button_box.append(delete_button)
        box.append(button_box)
        
        # Set the main container as the row's child
        self.set_child(box)
        
        # Show all children
        self.show_all()
    
    def on_edit_clicked(self, button):
        """Handle edit button click"""
        self.emit("edit-rule", self.rule)
    
    def on_delete_clicked(self, button):
        """Handle delete button click"""
        self.emit("delete-rule", self.rule)
    
    def update_ui(self):
        """Update the UI to reflect the current rule data"""
        if not hasattr(self, 'type_label') or not hasattr(self, 'details_label'):
            return
        
        rule_type = self.rule.get('type', 'local')
        listen_addr = self.rule.get('listen_addr', 'localhost')
        listen_port = self.rule.get('listen_port', '')
        
        try:
            # Update type label
            type_text = rule_type.capitalize()
            self.type_label.set_markup(f'<b>{type_text}</b>')
            
            # Update details based on rule type
            details = []
            
            if rule_type in ['local', 'remote']:
                # Local or Remote forwarding: show listen and remote addresses
                remote_host = self.rule.get('remote_host', 'localhost')
                remote_port = self.rule.get('remote_port', '')
                
                # Format the connection string
                listen_str = f"{listen_addr}:{listen_port}" if listen_port else listen_addr
                remote_str = f"{remote_host}:{remote_port}" if remote_port else remote_host
                
                if rule_type == 'local':
                    details.append(f"Local {listen_str} → Remote {remote_str}")
                else:  # remote
                    details.append(f"Remote {listen_str} → Local {remote_str}")
                
                # Add direction indicator
                direction = "🔵 Incoming" if rule_type == 'local' else "🔴 Outgoing"
                details.append(f"{direction} • {rule_type.capitalize()} Forwarding")
                
            else:  # dynamic
                # Dynamic forwarding: show SOCKS proxy info
                details.append(f"SOCKS5 Proxy on {listen_addr}:{listen_port}")
                details.append("🔵 Dynamic Port Forwarding")
            
            # Set the details text
            self.details_label.set_text(" • ".join(details))
            
            # Add tooltip with full details
            tooltip_text = "\n".join([
                f"Type: {rule_type.capitalize()}",
                f"Listen: {listen_addr}:{listen_port}",
            ])
            
            if rule_type in ['local', 'remote']:
                tooltip_text += f"\nRemote: {self.rule.get('remote_host', 'localhost')}:{self.rule.get('remote_port', '')}"
            
            self.set_tooltip_text(tooltip_text)
            
        except Exception as e:
            logger.error(f"Error updating rule row UI: {e}", exc_info=True)
            self.details_label.set_text("Error displaying rule")

class PortForwardingRules(Gtk.Box):
    """Widget for managing port forwarding rules"""
    __gtype_name__ = 'PortForwardingRules'
    
    __gsignals__ = {
        'changed': (GObject.SignalFlags.RUN_FIRST, None, ()),
    }
    
    def __init__(self, **kwargs):
        super().__init__(orientation=Gtk.Orientation.VERTICAL, spacing=12, **kwargs)
        self.rules: List[Dict[str, Any]] = []
        self._on_rule_changed = None
        
        # Create main container
        self.set_margin_top(12)
        self.set_margin_bottom(12)
        self.set_margin_start(12)
        self.set_margin_end(12)
        
        # Create scrolled window for rules list
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_vexpand(True)
        scrolled.set_hexpand(True)
        scrolled.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        
        # Create rules list
        self.rules_list = Gtk.ListBox()
        self.rules_list.set_selection_mode(Gtk.SelectionMode.NONE)
        self.rules_list.set_header_func(self.on_list_header_func)
        scrolled.set_child(self.rules_list)
        
        # Add button
        self.add_rule_button = Gtk.Button()
        self.add_rule_button.set_halign(Gtk.Align.CENTER)
        self.add_rule_button.set_margin_top(6)
        self.add_rule_button.set_margin_bottom(6)
        
        # Add button content
        button_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        button_box.append(Gtk.Image(icon_name='list-add-symbolic'))
        button_box.append(Gtk.Label(label='Add Port Forwarding Rule'))
        self.add_rule_button.set_child(button_box)
        self.add_rule_button.connect('clicked', self.on_add_rule_clicked)
        
        # Pack widgets
        self.append(scrolled)
        self.append(self.add_rule_button)
        
        # Connect signals
        self.connect('destroy', self.on_destroy)
        
        # Show all children
        self.show_all()
    
    def on_destroy(self, widget):
        """Clean up signal handlers when the widget is destroyed"""
        for row in self.rules_list:
            if hasattr(row, 'disconnect_by_func'):
                try:
                    row.disconnect_by_func(self.on_edit_rule)
                    row.disconnect_by_func(self.on_delete_rule)
                except (TypeError, ValueError):
                    pass
    
    def set_rules(self, rules: List[Dict[str, Any]]):
        """
        Set the list of forwarding rules.
        
        Args:
            rules: List of rule dictionaries to set
        """
        if not isinstance(rules, list):
            logger.warning(f"Expected list of rules, got {type(rules).__name__}")
            rules = []
            
        self.rules = []
        
        # Validate and normalize each rule
        for rule in rules:
            if not isinstance(rule, dict):
                logger.warning(f"Skipping invalid rule (not a dictionary): {rule}")
                continue
                
            # Create a normalized rule with default values
            normalized_rule = {
                'type': rule.get('type', 'local'),
                'enabled': bool(rule.get('enabled', True)),
                'listen_addr': str(rule.get('listen_addr', 'localhost')),
                'listen_port': int(rule.get('listen_port', 8080))
            }
            
            # Add remote host/port for local/remote forwarding
            if normalized_rule['type'] in ['local', 'remote']:
                normalized_rule.update({
                    'remote_host': str(rule.get('remote_host', 'localhost')),
                    'remote_port': int(rule.get('remote_port', 80))
                })
            
            self.rules.append(normalized_rule)
        
        # Update the UI
        self.update_rules_list()
    
    def get_rules(self) -> List[Dict[str, Any]]:
        """Get the current list of forwarding rules"""
        return self.rules.copy()
    
    def update_rules_list(self):
        """Update the rules list UI"""
        if not self.rules_list:
            return
        
        # Remove existing rows
        while row := self.rules_list.get_first_child():
            self.rules_list.remove(row)
        
        # Add current rules
        for rule in self.rules:
            self.add_rule_row(rule)
        
        # Emit changed signal
        self.emit('changed')
    
    def add_rule_row(self, rule: Dict[str, Any]):
        """
        Add a rule row to the list.
        
        Args:
            rule: The rule dictionary to add
        """
        if not self.rules_list:
            logger.warning("Cannot add rule row: rules_list is not initialized")
            return
        
        try:
            # Create row
            row = PortForwardingRuleRow(rule=rule)
            
            # Connect edit button
            edit_button = Gtk.Template.Child('edit_button')
            if edit_button:
                edit_button.connect('clicked', self.on_edit_rule_clicked, rule)
            
            # Connect delete button
            delete_button = Gtk.Template.Child('delete_button')
            if delete_button:
                delete_button.connect('clicked', self.on_delete_rule_clicked, rule)
            
            # Add to list
            self.rules_list.append(row)
            
            # Show the row
            row.show()
            
        except Exception as e:
            logger.error(f"Error adding rule row: {e}", exc_info=True)
    
    def on_list_header_func(self, row: Gtk.ListBoxRow, before: Optional[Gtk.ListBoxRow]):
        """Header function for the rules list"""
        if before:
            row.set_header(Gtk.Separator(orientation=Gtk.Orientation.HORIZONTAL))
    
    def on_add_rule_clicked(self, button: Gtk.Button):
        """Handle add rule button click"""
        dialog = PortForwardingRuleDialog(transient_for=self.get_root())
        dialog.connect('response', self.on_rule_dialog_response, None)
        dialog.present()
    
    def on_edit_rule_clicked(self, button: Gtk.Button, rule: Dict[str, Any]):
        """Handle edit rule button click"""
        dialog = PortForwardingRuleDialog(transient_for=self.get_root(), rule=rule)
        dialog.connect('response', self.on_rule_dialog_response, rule)
        dialog.present()
    
    def on_delete_rule_clicked(self, button: Gtk.Button, rule: Dict[str, Any]):
        """
        Handle delete rule button click.
        
        Args:
            button: The button that was clicked
            rule: The rule to delete
        """
        try:
            if rule in self.rules:
                self.rules.remove(rule)
                self.update_rules_list()
                self.emit('changed')
                logger.debug("Rule deleted")
        except Exception as e:
            logger.error(f"Error deleting rule: {e}", exc_info=True)
    
    def on_rule_dialog_response(self, dialog: 'PortForwardingRuleDialog', response: int, old_rule: Optional[Dict[str, Any]]):
        """
        Handle response from the rule dialog.
        
        Args:
            dialog: The dialog that emitted the response
            response: The response code
            old_rule: The original rule being edited, or None for a new rule
        """
        try:
            if response == Gtk.ResponseType.ACCEPT:
                # Get the updated rule from the dialog
                rule = dialog.get_rule()
                if not rule:
                    logger.warning("No valid rule data returned from dialog")
                    dialog.destroy()
                    return
                
                # Update existing rule or add new one
                if old_rule and old_rule in self.rules:
                    index = self.rules.index(old_rule)
                    self.rules[index] = rule
                    logger.debug(f"Updated rule at index {index}")
                else:
                    self.rules.append(rule)
                    logger.debug("Added new rule")
                
                # Update the UI
                self.update_rules_list()
                
                # Emit changed signal
                self.emit('changed')
            
            # Clean up the dialog
            dialog.destroy()
            
        except Exception as e:
            logger.error(f"Error handling rule dialog response: {e}", exc_info=True)
            if 'dialog' in locals():
                dialog.destroy()

@Gtk.Template(resource_path='/io/github/mfat/sshpilot/ui/port_forwarding_rules.ui')
class PortForwardingRuleDialog(Adw.Window):
    """Dialog for adding/editing port forwarding rules"""
    __gtype_name__ = 'PortForwardingRuleDialog'
    
    def __init__(self, transient_for: Optional[Gtk.Window] = None, rule: Optional[Dict[str, Any]] = None, **kwargs):
        super().__init__(transient_for=transient_for, **kwargs)
        self.rule = rule or {}
        self.setup_ui()
    
    def setup_ui(self):
        """Set up the dialog UI"""
        # Connect signals
        cancel_button = self.get_template_child(PortForwardingRuleDialog, 'cancel_button')
        save_button = self.get_template_child(PortForwardingRuleDialog, 'save_button')
        type_row = self.get_template_child(PortForwardingRuleDialog, 'type_row')
        
        if cancel_button:
            cancel_button.connect('clicked', lambda *_: self.response(Gtk.ResponseType.CANCEL))
        if save_button:
            save_button.connect('clicked', self.on_save_clicked)
        if type_row:
            type_row.connect('notify::selected', self.on_type_changed)
        
        # Set up validation
        self.setup_validation()
        
        # Load rule data if editing
        if self.rule:
            self.load_rule()
        else:
            # Default to local forwarding
            self.rule = {
                'type': 'local',
                'enabled': True,
                'listen_addr': 'localhost',
                'listen_port': 8080,
                'remote_host': 'localhost',
                'remote_port': 80
            }
            self.update_ui()
    
    def setup_validation(self):
        """Set up input validation"""
        # Listen port validation
        listen_port_row = self.get_template_child(PortForwardingRuleDialog, 'listen_port_row')
        if listen_port_row:
            listen_port_row.connect('changed', self.validate_input)
        
        # Remote port validation (for local/remote forwarding)
        remote_port_row = self.get_template_child(PortForwardingRuleDialog, 'remote_port_row')
        if remote_port_row:
            remote_port_row.connect('changed', self.validate_input)
        
        # Remote host validation (for local/remote forwarding)
        remote_host_row = self.get_template_child(PortForwardingRuleDialog, 'remote_host_row')
        if remote_host_row:
            remote_host_row.connect('changed', self.validate_input)
    
    def validate_input(self, *args) -> bool:
        """
        Validate the form inputs and show error messages if needed.
        
        Returns:
            bool: True if all inputs are valid, False otherwise
        """
        errors = []
        
        # Get UI elements
        type_row = self.get_template_child(PortForwardingRuleDialog, 'type_row')
        listen_addr_row = self.get_template_child(PortForwardingRuleDialog, 'listen_addr_row')
        listen_port_row = self.get_template_child(PortForwardingRuleDialog, 'listen_port_row')
        remote_host_row = self.get_template_child(PortForwardingRuleDialog, 'remote_host_row')
        remote_port_row = self.get_template_child(PortForwardingRuleDialog, 'remote_port_row')
        
        # Basic validation of UI elements
        if not all([type_row, listen_addr_row, listen_port_row, remote_host_row, remote_port_row]):
            logger.error("Missing required UI elements for validation")
            return False
        
        # Get values
        rule_type_idx = type_row.get_selected()
        listen_addr = listen_addr_row.get_text().strip()
        listen_port_str = listen_port_row.get_text().strip()
        remote_host = remote_host_row.get_text().strip()
        remote_port_str = remote_port_row.get_text().strip()
        
        # Validate listen address
        if not listen_addr:
            errors.append("Listen address cannot be empty")
        
        # Validate listen port
        try:
            listen_port = int(listen_port_str)
            if listen_port < 1 or listen_port > 65535:
                errors.append("Listen port must be between 1 and 65535")
        except (ValueError, TypeError):
            errors.append("Listen port must be a valid number")
        
        # For local and remote forwarding, validate remote host and port
        if rule_type_idx in [0, 1]:  # Local or Remote forwarding
            # Validate remote host
            if not remote_host:
                errors.append("Remote host cannot be empty")
            
            # Validate remote port
            try:
                remote_port = int(remote_port_str)
                if remote_port < 1 or remote_port > 65535:
                    errors.append("Remote port must be between 1 and 65535")
            except (ValueError, TypeError):
                errors.append("Remote port must be a valid number")
        
        # Update error message
        error_label = self.get_template_child(PortForwardingRuleDialog, 'error_label')
        if error_label:
            if errors:
                error_label.set_visible(True)
                error_label.set_label("\n".join([f"• {error}" for error in errors]))
                error_label.add_css_class('error')
            else:
                error_label.set_visible(False)
                error_label.set_label("")
                error_label.remove_css_class('error')
        
        # Update save button state
        save_button = self.get_template_child(PortForwardingRuleDialog, 'save_button')
        if save_button:
            save_button.set_sensitive(not bool(errors))
        
        return not bool(errors)
    
    def load_rule(self):
        """Load rule data into the form"""
        type_row = self.get_template_child(PortForwardingRuleDialog, 'type_row')
        listen_addr_row = self.get_template_child(PortForwardingRuleDialog, 'listen_addr_row')
        listen_port_row = self.get_template_child(PortForwardingRuleDialog, 'listen_port_row')
        remote_host_row = self.get_template_child(PortForwardingRuleDialog, 'remote_host_row')
        remote_port_row = self.get_template_child(PortForwardingRuleDialog, 'remote_port_row')
        
        if not all([type_row, listen_addr_row, listen_port_row, remote_host_row, remote_port_row]):
            return
        
        # Set type
        rule_type = self.rule.get('type', 'local')
        if rule_type == 'local':
            type_row.set_selected(0)
        elif rule_type == 'remote':
            type_row.set_selected(1)
        else:  # dynamic
            type_row.set_selected(2)
        
        # Set fields
        listen_addr_row.set_text(self.rule.get('listen_addr', 'localhost'))
        listen_port_row.set_value(self.rule.get('listen_port', 8080))
        
        remote_host = self.rule.get('remote_host', 'localhost')
        remote_port = self.rule.get('remote_port', 80)
        
        remote_host_row.set_text(remote_host)
        remote_port_row.set_value(remote_port)
        
        # Update UI based on type
        self.update_ui()
    
    def update_ui(self):
        """Update the UI based on the selected rule type"""
        type_row = self.get_template_child(PortForwardingRuleDialog, 'type_row')
        remote_host_row = self.get_template_child(PortForwardingRuleDialog, 'remote_host_row')
        remote_port_row = self.get_template_child(PortForwardingRuleDialog, 'remote_port_row')
        
        if not all([type_row, remote_host_row, remote_port_row]):
            return
        
        rule_type = type_row.get_selected()
        
        # Show/hide fields based on rule type
        if rule_type in [0, 1]:  # Local or Remote forwarding
            remote_host_row.set_visible(True)
            remote_port_row.set_visible(True)
        else:  # Dynamic forwarding
            remote_host_row.set_visible(False)
            remote_port_row.set_visible(False)
        
        # Update validation
        self.validate_input()
    
    def on_type_changed(self, combo_row: Adw.ComboRow, *args):
        """Handle rule type change"""
        self.update_ui()
    
    def on_save_clicked(self, button: Gtk.Button):
        """
        Handle save button click.
        
        Validates the form and accepts the dialog if validation passes.
        """
        if self.validate_input():
            self.response(Gtk.ResponseType.ACCEPT)
        else:
            # The error message is already shown in the validation method
            # Just ensure the error is visible by scrolling to it
            error_label = self.get_template_child(PortForwardingRuleDialog, 'error_label')
            if error_label and error_label.get_visible():
                error_label.grab_focus()
                # If we had a scrolled window, we would scroll to the error here
                # scrolled_window = error_label.get_ancestor(Gtk.ScrolledWindow)
                # if scrolled_window:
                #     scrolled_window.get_vadjustment().set_value(error_label.get_allocation().y)
    
    def get_rule(self) -> Dict[str, Any]:
        """Get the rule data from the form"""
        type_row = self.get_template_child(PortForwardingRuleDialog, 'type_row')
        listen_addr_row = self.get_template_child(PortForwardingRuleDialog, 'listen_addr_row')
        listen_port_row = self.get_template_child(PortForwardingRuleDialog, 'listen_port_row')
        remote_host_row = self.get_template_child(PortForwardingRuleDialog, 'remote_host_row')
        remote_port_row = self.get_template_child(PortForwardingRuleDialog, 'remote_port_row')
        
        if not all([type_row, listen_addr_row, listen_port_row, remote_host_row, remote_port_row]):
            return {}
        
        # Determine rule type
        rule_type_idx = type_row.get_selected()
        if rule_type_idx == 0:
            rule_type = 'local'
        elif rule_type_idx == 1:
            rule_type = 'remote'
        else:
            rule_type = 'dynamic'
        
        # Build rule dictionary
        rule = {
            'type': rule_type,
            'enabled': True,
            'listen_addr': listen_addr_row.get_text().strip() or 'localhost',
            'listen_port': int(listen_port_row.get_value())
        }
        
        # Add remote host/port for local/remote forwarding
        if rule_type in ['local', 'remote']:
            rule.update({
                'remote_host': remote_host_row.get_text().strip() or 'localhost',
                'remote_port': int(remote_port_row.get_value())
            })
        
        return rule
