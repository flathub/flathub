#!/usr/bin/env python3

import sys
import os
import logging
import json
import threading
import time
import urllib.request
import subprocess
import concurrent.futures
import tempfile
import gi
from datetime import datetime

gi.require_version('Gtk', '4.0')
gi.require_version('Adw', '1')
gi.require_version('Gdk', '4.0')
gi.require_version('GdkPixbuf', '2.0')
gi.require_version('Pango', '1.0')
from gi.repository import GLib, Gtk, Gio, Adw, Gdk, GdkPixbuf, Pango

# Add the current directory to the Python path
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from wselector.models import WallpaperInfo, WallpaperGObject
from wselector.api import WSelectorScraper
from wselector.utils import download_thumbnail

# Setup logging
def setup_logging():
    """Configure logging with both file and console handlers"""
    # Create logs directory if it doesn't exist
    log_dir = os.path.join(GLib.get_user_cache_dir(), "wselector", "logs")
    os.makedirs(log_dir, exist_ok=True)
    
    # Create a custom logger
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    
    # Create handlers
    log_file = os.path.join(log_dir, "wselector.log")
    file_handler = logging.FileHandler(log_file, mode='w')
    file_handler.setLevel(logging.DEBUG)
    
    console_handler = logging.StreamHandler()
    console_handler.setLevel(logging.INFO)
    
    # Create formatters and add it to handlers
    log_format = '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    formatter = logging.Formatter(log_format)
    file_handler.setFormatter(formatter)
    console_handler.setFormatter(formatter)
    
    # Clear any existing handlers
    if logger.hasHandlers():
        logger.handlers.clear()
    
    # Add handlers to the logger
    logger.addHandler(file_handler)
    logger.addHandler(console_handler)
    
    return logger

# Initialize logging
logger = setup_logging()

# Configuration paths
CONFIG_PATH = os.path.join(GLib.get_user_config_dir(), "wselector", "config.json")
CACHE_DIR = os.path.join(GLib.get_user_cache_dir(), "wselector")
CACHE_INDEX = os.path.join(CACHE_DIR, "cache_index.json")

# Default configuration
DEFAULT_CONFIG = {
    "categories": "111",
    "purity": "100",
    "selected_categories": ["General"],
    "selected_purity": ["SFW"],
    "sort_mode": "latest",
    "theme": "light"  # Added theme with default light mode
}

# Ensure cache directory exists
os.makedirs(CACHE_DIR, exist_ok=True)

class WSelectorApp(Adw.Application):
    def __init__(self, application_id, flags):
        super().__init__(application_id=application_id, flags=flags)
        GLib.set_application_name("WSelector")
        GLib.set_prgname(application_id)

        # Initialize configuration
        self.config = DEFAULT_CONFIG.copy()
        self.load_config()  # This will update self.config with saved values
        
        # Initialize theme state
        self.is_dark_theme = self.config.get("theme", "light") == "dark"
        
        # Initialize other instance variables
        self.current_page = 1
        self.loading = False
        self.current_query = None  # Store the current search query
        self.min_column_width = 200
        self.column_spacing = 10
        self.row_spacing = 10
        self.margin = 10
        self.scroll_position = 0  # Track scroll position for refresh
        
        # Schedule theme application after the main loop starts
        GLib.idle_add(self.apply_saved_theme)

        # Search debounce timer
        self.search_timeout_id = None
        self.search_delay_ms = 800  # 800ms delay - increased to accommodate fast typing

        self.connect("notify::default-width", self.on_window_size_changed)
        self.connect("notify::default-height", self.on_window_size_changed)

        self.create_action("preferences", self.on_preferences)
        self.create_action("view_downloads", self.on_view_downloads)
        self.create_action("about", self.on_about)

    def create_action(self, name, callback, shortcuts=None):
        action = Gio.SimpleAction.new(name, None)
        action.connect("activate", callback)
        self.add_action(action)
        if shortcuts:
            self.set_accels_for_action(f"app.{name}", shortcuts)

    def create_search_bar(self):
        """Create and set up the search bar and filters with revealer for fade effect."""
        # Create revealer for smooth fade in/out
        self.search_revealer = Gtk.Revealer()
        self.search_revealer.set_transition_type(Gtk.RevealerTransitionType.CROSSFADE)
        self.search_revealer.set_transition_duration(200)  # 200ms for smooth fade
        self.search_revealer.set_reveal_child(True)  # Start revealed
        
        # Create search bar container
        search_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        search_box.set_margin_top(6)
        search_box.set_margin_bottom(6)
        search_box.set_margin_start(12)
        search_box.set_margin_end(12)
        
        # Set the search box as the revealer's child
        self.search_revealer.set_child(search_box)
        
        # Track scroll state
        self.last_scroll_position = 0
        self.header_visible = True
        
        # Create search entry with refresh button
        search_entry_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        
        # Create home button
        self.home_button = Gtk.Button()
        self.home_button.set_icon_name("go-home-symbolic")
        self.home_button.set_tooltip_text("Scroll to top")
        self.home_button.set_valign(Gtk.Align.CENTER)
        self.home_button.connect("clicked", self.on_home_clicked)
        search_entry_box.append(self.home_button)
        
        # Add separator between home button and search
        separator = Gtk.Separator(orientation=Gtk.Orientation.VERTICAL)
        separator.set_margin_start(6)
        separator.set_margin_end(6)
        search_entry_box.append(separator)
        
        # Create search entry
        self.search_entry = Gtk.SearchEntry()
        self.search_entry.set_placeholder_text("Search wallpapers...")
        self.search_entry.set_hexpand(True)
        self.search_entry.connect("search-changed", self.on_search_changed)
        self.search_entry.connect("stop-search", self.on_search_stopped)
        search_entry_box.append(self.search_entry)
        
        # Create refresh button
        self.refresh_button = Gtk.Button()
        self.refresh_button.set_icon_name("view-refresh-symbolic")
        self.refresh_button.set_tooltip_text("Refresh results")
        self.refresh_button.set_valign(Gtk.Align.CENTER)
        self.refresh_button.connect("clicked", self.on_refresh_clicked)
        search_entry_box.append(self.refresh_button)
        
        search_box.append(search_entry_box)
        
        # Add sort dropdown
        sort_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        sort_label = Gtk.Label(label="Sort by:")
        sort_label.set_valign(Gtk.Align.CENTER)
        
        # Use the original sort items
        self.sort_items = ["Latest", "Popular", "Random"]
        self.sort_selector = Gtk.DropDown.new_from_strings(self.sort_items)
        self.sort_selector.set_selected(0)  # Default to Latest
        self.sort_selector.connect("notify::selected", self.on_sort_changed)
        
        sort_box.append(sort_label)
        sort_box.append(self.sort_selector)
        search_box.append(sort_box)
        
        # Create a container for the search bar and separator
        self.search_container = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        
        # Add separator at the bottom of the search bar
        self.separator = Gtk.Separator()
        
        # Add search bar and separator to container
        self.search_container.append(self.search_revealer)
        self.search_container.append(self.separator)
        
        # Set initial opacity for smooth transitions
        self.search_revealer.set_opacity(1.0)
        self.separator.set_opacity(1.0)
        
        # Create a spinner for loading indicators
        self.spinner = Gtk.Spinner()
        self.spinner.set_size_request(24, 24)
        self.spinner.hide()
        
        # Add spinner to a box to control its position
        spinner_box = Gtk.Box(halign=Gtk.Align.END, margin_end=12, margin_top=12)
        spinner_box.append(self.spinner)
        
        # Add container to main window
        self.main_box.prepend(self.search_container)
        #self.main_box.prepend(spinner_box) Not Really sure about this one, but need it maybe later.
        
    def on_home_clicked(self, button):
        """Handle home button click event - scroll to top of the page."""
        if hasattr(self, 'scroll') and self.scroll:
            self.scroll.get_vadjustment().set_value(0)
    
    def on_refresh_clicked(self, button):
        """Handle refresh button click event."""
        logger.info("Refresh button clicked, reloading wallpapers")
        # Save current scroll position
        self.scroll_position = self.scroll.get_vadjustment().get_value()
        # Trigger a new search with the current query
        self.current_page = 1  # Reset to first page
        self.search_wallpapers(refresh=True)
        
        # Restore scroll position after a short delay to allow UI to update
        def restore_scroll():
            if hasattr(self, 'scroll') and self.scroll:
                self.scroll.get_vadjustment().set_value(self.scroll_position)
        GLib.timeout_add(300, restore_scroll)
    
    def do_activate(self):
        """Set up the application's main window."""
        # Check if window already exists
        if hasattr(self, 'win') and self.win is not None:
            if self.win.is_visible():
                self.win.present()
                return
            else:
                self.win.destroy()
        
        # Create the main window
        self.win = Gtk.ApplicationWindow(application=self)
        self.win.set_title("WSelector")
        self.win.set_default_size(800, 650)
        
        # Initialize scraper
        self.scraper = WSelectorScraper()
        
        # Create toast overlay
        self.toast_overlay = Adw.ToastOverlay()
        
        # Main vertical box
        self.main_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.toast_overlay.set_child(self.main_box)
        
        # Set the toast overlay as the window's child
        self.win.set_child(self.toast_overlay)
        
        # Apply saved theme after the main loop has started
        GLib.idle_add(self.apply_saved_theme)
        
        # Create header bar with title, theme toggle, and menu
        header = Gtk.HeaderBar()
        header.set_show_title_buttons(True)
        header.set_title_widget(Gtk.Label(label="WSelector"))
        self.win.set_titlebar(header)
        
        # Create search bar
        self.create_search_bar()
        
        # Create scrolled window for the flowbox
        self.scroll = Gtk.ScrolledWindow()
        self.scroll.set_hexpand(True)
        self.scroll.set_vexpand(True)
        
        # Create flowbox for wallpapers with responsive grid layout
        self.flowbox = Gtk.FlowBox()
        self.flowbox.set_valign(Gtk.Align.START)
        self.flowbox.set_homogeneous(True)
        self.flowbox.set_selection_mode(Gtk.SelectionMode.NONE)
        self.flowbox.set_column_spacing(12)
        self.flowbox.set_row_spacing(12)
        self.flowbox.set_margin_top(12)
        self.flowbox.set_margin_bottom(12)
        self.flowbox.set_margin_start(12)
        self.flowbox.set_margin_end(12)
        self.flowbox.set_halign(Gtk.Align.CENTER)
        self.flowbox.set_hexpand(True)
        self.flowbox.set_vexpand(True)

        # Create overlay for the flowbox
        overlay = Gtk.Overlay()
        overlay.set_child(self.flowbox)

        # Create scrolled window
        self.scroll = Gtk.ScrolledWindow()
        self.scroll.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        self.scroll.set_child(overlay)
        self.scroll.get_vadjustment().connect("value-changed", self.on_scroll_changed)
        
        # Add scroll window to main box
        self.main_box.append(self.scroll)
        
        # Connect scroll events
        vadj = self.scroll.get_vadjustment()
        vadj.connect("value-changed", self.on_scroll_changed)
        
        # Get theme preference
        theme = self.config.get("theme", "light")
        
        # Create theme toggle button with correct initial state
        self.theme_button = Gtk.ToggleButton()
        if theme == "dark":
            self.theme_button.set_icon_name("weather-clear-night-symbolic")
            self.theme_button.set_active(True)
            Adw.StyleManager.get_default().set_color_scheme(Adw.ColorScheme.FORCE_DARK)
        else:
            self.theme_button.set_icon_name("weather-clear-symbolic")
            self.theme_button.set_active(False)
            Adw.StyleManager.get_default().set_color_scheme(Adw.ColorScheme.FORCE_LIGHT)
            
        # Connect the signal after setting the initial state
        self.theme_button.connect("toggled", self.toggle_theme)
        header.pack_end(self.theme_button)

        # Menu
        menu = Gio.Menu()
        menu.append("Preferences", "app.preferences")
        menu.append("View Downloads", "app.view_downloads")
        menu.append("About", "app.about")
        menu_button = Gtk.MenuButton(icon_name="open-menu-symbolic")
        menu_button.set_menu_model(menu)
        header.pack_end(menu_button)
        
        # Load preferences
        self.load_preferences()
        
        # Show the window
        self.win.present()
        
        # Load initial wallpapers
        self.load_wallpapers()
        self.win.set_child(self.main_box)
        
        # Initialize scraper if not already done
        if not hasattr(self, 'scraper'):
            self.scraper = WSelectorScraper()
            
        # Only load preferences if not already loaded
        if not hasattr(self, '_preferences_loaded') or not self._preferences_loaded:
            self.load_preferences()
            self.save_preferences()
            self._preferences_loaded = True
        
        # Only load wallpapers if not already loading
        if not self.loading:
            self.load_wallpapers()

        self.win.present()

    def on_search_changed(self, entry):
        """Handle search entry changes"""
        # Get the search text
        query = entry.get_text().strip()
        
        logger.info(f"Search changed - Previous query: '{self.current_query}', New text: '{query}'")
        
        # Store the current query (or None if empty)
        self.current_query = query if query else None
        
        logger.info(f"Search changed - Updated current_query to: '{self.current_query}'")
        
        # Cancel any previous search timeout
        if self.search_timeout_id:
            GLib.source_remove(self.search_timeout_id)
            self.search_timeout_id = None
        
        # Show spinner to indicate search is pending
        if hasattr(self, 'spinner') and self.spinner is not None:
            try:
                self.spinner.start()
                self.spinner.show()
            except Exception as e:
                logger.error(f"Error starting spinner: {e}")
        
        # Set a timeout to perform the search after typing stops
        self.search_timeout_id = GLib.timeout_add(self.search_delay_ms, self.perform_search)
        
    def _reset_scroll_position(self):
        """Reset the scroll position to the top."""
        if hasattr(self, 'scroll'):
            adj = self.scroll.get_vadjustment()
            adj.set_value(0)
            self.scroll.queue_draw()
            logger.debug("Scroll position reset to top")

    def _clear_thumbnail_cache(self):
        """Clear the thumbnail cache and reset related state."""
        logger.info("Clearing thumbnail cache")
        if hasattr(self, '_thumbnail_widgets'):
            # Clear the thumbnail widgets dictionary
            self._thumbnail_widgets.clear()
        
        # Clear any pending thumbnail loads
        if hasattr(self, '_thumbnail_executor') and self._thumbnail_executor:
            self._thumbnail_executor.shutdown(wait=False)
            self._thumbnail_executor = None
        
        # Clear any pending futures
        if hasattr(self, '_thumbnail_futures'):
            for future in self._thumbnail_futures:
                if not future.done():
                    future.cancel()
            self._thumbnail_futures.clear()
    
    def perform_search(self):
        """Perform the actual search after the debounce delay"""
        logger.info(f"Performing search with query: '{self.current_query}'")
        
        # Reset the timeout ID
        self.search_timeout_id = None
        
        # Clear existing thumbnails and cache
        self._clear_thumbnail_cache()
        
        # Reset to page 1 and clear the flowbox
        self.current_page = 1
        self.flowbox.remove_all()
        
        # Reset scroll position to top
        self._reset_scroll_position()
        
        # Load wallpapers with the current query
        # If query is empty, explicitly pass None to trigger reload with saved preferences
        if not self.current_query:
            logger.info("Search text cleared, reloading with saved preferences")
            self.load_wallpapers(query=None)
        else:
            logger.info(f"Search changed - Loading wallpapers with query: '{self.current_query}'")
            self.load_wallpapers(query=self.current_query)
            
        # Return False to prevent the timeout from repeating
        return False

    def on_refresh_clicked(self, button):
        """Handle refresh button click."""
        logger.info("Refresh button clicked")
        
        # Reset scroll position to top
        self._reset_scroll_position()
            
        self.load_wallpapers(query=self.current_query, page=1, force_reload=True)
        
        # Show a toast notification
        toast = Adw.Toast.new("Wallpapers refreshed")
        toast.set_timeout(2)  # 2 seconds
        self.toast_overlay.add_toast(toast)
        
    def on_search_stopped(self, entry):
        """Handle the clear button click event in the search entry."""
        logger.info("Search stopped, reloading with saved preferences")
        entry.set_text("")  # Clear the search text
        self.current_query = None  # Clear the current query
        
        # Cancel any pending search timeout
        if self.search_timeout_id:
            GLib.source_remove(self.search_timeout_id)
            self.search_timeout_id = None
            
        # Reset to page 1 and clear the flowbox
        self.current_page = 1
        self.flowbox.remove_all()
        
        # Reset scroll position to top
        if hasattr(self, 'scroll'):
            self.scroll.get_vadjustment().set_value(0)
        
        # Immediately load wallpapers with no query
        self.load_wallpapers(query=None)

    def on_sort_changed(self, dropdown, pspec):
        selected_item = dropdown.get_selected_item()
        if selected_item:
            sort_mode = selected_item.get_string().lower()
            self.config["sort_mode"] = sort_mode
            self.save_preferences()
            # Force a full reload with the new sort mode
            self.current_page = 1
            # Clear existing wallpapers
            child = self.flowbox.get_first_child()
            while child is not None:
                next_child = child.get_next_sibling()
                self.flowbox.remove(child)
                child = next_child
            # Reset scroll position to top
            self._reset_scroll_position()
            # Reload with current query and first page
            self.load_wallpapers(query=self.current_query if self.current_query else None, page=1, force_reload=True)


    
    def apply_saved_theme(self):
        """Apply the saved theme preference."""
        try:
            style_manager = Adw.StyleManager.get_default()
            if not style_manager:
                logger.warning("StyleManager not available yet, retrying...")
                GLib.timeout_add(100, self.apply_saved_theme)
                return
                
            if self.is_dark_theme:
                style_manager.set_color_scheme(Adw.ColorScheme.FORCE_DARK)
            else:
                style_manager.set_color_scheme(Adw.ColorScheme.FORCE_LIGHT)
                
            logger.info(f"Applied saved theme: {'dark' if self.is_dark_theme else 'light'}")
            return False  # Remove the timeout
        except Exception as e:
            logger.error(f"Error applying theme: {e}")
            return False
    
    def toggle_theme(self, button):
        style_manager = Adw.StyleManager.get_default()
        if button.get_active():
            # Set dark theme
            style_manager.set_color_scheme(Adw.ColorScheme.FORCE_DARK)
            button.set_icon_name("weather-clear-night-symbolic")
            # Save preference to config
            self.config["theme"] = "dark"
        else:
            # Set light theme
            style_manager.set_color_scheme(Adw.ColorScheme.FORCE_LIGHT)
            button.set_icon_name("weather-clear-symbolic")
            # Save preference to config
            self.config["theme"] = "light"
            
        # Save config to file
        self.save_config()
        logger.info(f"Theme preference saved: {self.config['theme']}")
        
        # Show a toast notification to confirm theme change
        toast = Adw.Toast.new(f"Theme changed to {self.config['theme']}")
        toast.set_timeout(2)
        self.toast_overlay.add_toast(toast)
        
    def save_config(self):
        """Save current configuration to the config file."""
        try:
            os.makedirs(os.path.dirname(CONFIG_PATH), exist_ok=True)
            with open(CONFIG_PATH, 'w') as f:
                json.dump(self.config, f, indent=4)
            logger.info(f"Configuration saved to {CONFIG_PATH}")
            return True
        except Exception as e:
            logger.error(f"Error saving configuration: {e}", exc_info=True)
            self.show_error(f"Failed to save configuration: {str(e)}")
            return False
            
    def load_preferences(self):
        try:
            if os.path.exists(CONFIG_PATH):
                with open(CONFIG_PATH, 'r') as f:
                    self.config = json.load(f)
            else:
                self.config = DEFAULT_CONFIG.copy()
        except Exception as e:
            logger.error(f"Error loading preferences: {e}")
            self.config = DEFAULT_CONFIG.copy()

    def load_config(self):
        """Load configuration from file or use defaults."""
        try:
            # Only load config if not already loaded
            if hasattr(self, '_config_loaded') and self._config_loaded:
                return
                
            self.config = DEFAULT_CONFIG.copy()
            
            if os.path.exists(CONFIG_PATH):
                try:
                    with open(CONFIG_PATH, 'r') as f:
                        loaded_config = json.load(f)
                        
                    # Validate and update configuration
                    if isinstance(loaded_config, dict):
                        # Ensure required keys exist with proper types
                        if "selected_categories" in loaded_config and isinstance(loaded_config["selected_categories"], list):
                            self.config["selected_categories"] = loaded_config["selected_categories"]
                        if "selected_purity" in loaded_config and isinstance(loaded_config["selected_purity"], list):
                            self.config["selected_purity"] = loaded_config["selected_purity"]
                        if "sort_mode" in loaded_config and isinstance(loaded_config["sort_mode"], str):
                            self.config["sort_mode"] = loaded_config["sort_mode"]
                        if "categories" in loaded_config and isinstance(loaded_config["categories"], str):
                            self.config["categories"] = loaded_config["categories"]
                        if "purity" in loaded_config and isinstance(loaded_config["purity"], str):
                            self.config["purity"] = loaded_config["purity"]
                        if "resolution" in loaded_config and loaded_config["resolution"]:
                            self.config["resolution"] = loaded_config["resolution"]
                        if "theme" in loaded_config and isinstance(loaded_config["theme"], str):
                            self.config["theme"] = loaded_config["theme"]
                            # Update the theme state to match the loaded config
                            self.is_dark_theme = self.config["theme"] == "dark"
                except json.JSONDecodeError:
                    logger.warning("Invalid config file, using defaults")
                except Exception as e:
                    logger.error(f"Error loading config: {e}")
            
            # Ensure we have at least one category and purity selected
            if not self.config["selected_categories"]:
                self.config["selected_categories"] = ["General"]
            if not self.config["selected_purity"]:
                self.config["selected_purity"] = ["SFW"]
                
            logger.debug(f"Current config: {self.config}")
            self._config_loaded = True
            
        except Exception as e:
            logger.error(f"Unexpected error loading configuration: {e}", exc_info=True)
            self.show_error(f"Failed to load configuration: {str(e)}")
            # Continue with default config

    def save_preferences(self):
        """Save current preferences to the config file."""
        try:
            # Ensure config directory exists
            config_dir = os.path.dirname(CONFIG_PATH)
            os.makedirs(config_dir, exist_ok=True)
            
            # Ensure we have default values if they're missing
            self.config.setdefault("selected_categories", ["General"])
            self.config.setdefault("selected_purity", ["SFW"])
            self.config.setdefault("categories", "100")
            self.config.setdefault("purity", "100")
            self.config.setdefault("sort_mode", "latest")
            
            # Write to a temporary file first, then atomically rename
            temp_path = f"{CONFIG_PATH}.tmp"
            with open(temp_path, 'w') as f:
                json.dump(self.config, f, indent=4)
                f.flush()
                os.fsync(f.fileno())
            
            # Atomic rename to ensure we don't corrupt the file
            os.replace(temp_path, CONFIG_PATH)
            logger.info(f"Successfully saved preferences to {CONFIG_PATH}")
            return True
            
        except Exception as e:
            logger.error(f"Error saving preferences: {e}", exc_info=True)
            # Try to clean up temp file if it exists
            if 'temp_path' in locals() and os.path.exists(temp_path):
                try:
                    os.remove(temp_path)
                except:
                    pass
            return False
    
            self.config.setdefault("selected_categories", ["General"])
            self.config.setdefault("selected_purity", ["SFW"])
            self.config.setdefault("categories", "100")
            self.config.setdefault("purity", "100")
            self.config.setdefault("sort_mode", "latest")
            
            # Apply sort mode
            sort_mode = self.config.get("sort_mode", "latest").lower()
            for i, sort_item in enumerate(self.sort_items):
                if sort_item.lower() == sort_mode:
                    self.sort_selector.set_selected(i)
                    break
            else:
                # Default to latest if no match found
                self.sort_selector.set_selected(0)
            
            # Apply categories if buttons exist
            if hasattr(self, 'category_buttons'):
                # Get the current categories from config
                categories_str = self.config.get("categories", "100")
                if len(categories_str) != 3:
                    categories_str = "100"
                
                # Map to checkboxes (General, Anime, People)
                category_mapping = [
                    ("general", 0),  # First character in categories_str
                    ("anime", 1),    # Second character
                    ("people", 2)    # Third character
                ]
                
                for cat_name, idx in category_mapping:
                    if cat_name in self.category_buttons:
                        is_active = len(categories_str) > idx and categories_str[idx] == '1'
                        self.category_buttons[cat_name].set_active(is_active)
            
            # Apply purity if buttons exist
            if hasattr(self, 'purity_buttons'):
                # Get the current purity from config
                purity_str = self.config.get("purity", "100")
                if len(purity_str) != 3:
                    purity_str = "100"
                
                # Map to checkboxes (SFW, Sketchy)
                purity_mapping = [
                    ("sfw", 0),      # First character in purity_str
                    ("sketchy", 1)   # Second character
                ]
                
                for pur_name, idx in purity_mapping:
                    if pur_name in self.purity_buttons:
                        is_active = len(purity_str) > idx and purity_str[idx] == '1'
                        self.purity_buttons[pur_name].set_active(is_active)
                    
        except Exception as e:
            logger.error(f"Error applying preferences: {e}", exc_info=True)
            # Reset to safe defaults on error
            if hasattr(self, 'category_buttons'):
                for btn in self.category_buttons.values():
                    btn.set_active(False)
                if 'general' in self.category_buttons:
                    self.category_buttons['general'].set_active(True)
                    
            if hasattr(self, 'purity_buttons'):
                for btn in self.purity_buttons.values():
                    btn.set_active(False)
                if 'sfw' in self.purity_buttons:
                    self.purity_buttons['sfw'].set_active(True)
            
            # Check if any relevant preferences changed that would affect the wallpapers display
            new_sort_mode = self.config.get("sort_mode", "latest").lower()
            new_categories = self.config.get("categories", "100")
            new_purity = self.config.get("purity", "100")
            
            # If any of these changed and reload_wallpapers is True, reload the wallpapers
            if reload_wallpapers and (old_sort_mode != new_sort_mode or 
                old_categories != new_categories or 
                old_purity != new_purity):
                logger.info(f"Preferences changed, reloading wallpapers")
                self.current_page = 1
                self.flowbox.remove_all()
                
                # Debug logging for search query state
                logger.info(f"apply_preferences - Before reload - Current search query: '{self.current_query}'")
                
                # Preserve the current search query when reloading after preference changes
                if self.current_query:
                    logger.info(f"Preferences changed, reloading wallpapers with search query: '{self.current_query}'")
                    # Explicitly pass the current query to ensure it's used
                    current_query_value = self.current_query
                    logger.info(f"Explicitly passing query value: '{current_query_value}' to load_wallpapers")
                    self.load_wallpapers(query=current_query_value, force_reload=True)
                else:
                    logger.info("Preferences changed, reloading wallpapers (no search query)")
                    self.load_wallpapers(force_reload=True)
    
    def load_wallpapers(self, query=None, page=1, prefetch=False, force_reload=False):
        """Load wallpapers with the current filters and page.
        
        Args:
            query: Search query string or None for default
            page: Page number to load (1-based)
            prefetch: If True, load in background without showing spinner
            force_reload: If True, force a reload even if already loading
        """
        # Skip if already loading the same page and not forcing a reload
        if (self.loading and not force_reload and 
            hasattr(self, 'current_query') and self.current_query == query and 
            hasattr(self, 'current_page') and self.current_page == page):
            logger.debug("Skipping duplicate load request")
            return
            
        logger.info(f"Loading wallpapers - Query: {query}, Sort: {self.config.get('sort_mode', 'latest')}, Page: {page}, Categories: {self.config.get('categories', '100')}, Purity: {self.config.get('purity', '100')}")
                   
        # Update current state
        self.current_query = query if query is not None else ""
        self.current_page = page
        
        # Handle loading state
        if self.loading and not (prefetch or force_reload):
            logger.debug("Already loading, skipping duplicate request")
            return
            
        # Update the current query if provided
        if query is not None:
            self.current_query = query if query else ""
            logger.info(f"Setting current_query to: '{self.current_query}'")
        elif self.current_query is None:
            # Initialize with empty string if no query provided
            self.current_query = ""
            logger.info("No query provided, using empty string for API call")
            
        # If it's the first page or force_reload is True, clear existing wallpapers
        if (page == 1 or force_reload) and not prefetch:
            logger.info("Loading first page, clearing existing wallpapers")
            # Clear the flowbox - GTK4 compatible way
            child = self.flowbox.get_first_child()
            while child is not None:
                next_child = child.get_next_sibling()
                self.flowbox.remove(child)
                child = next_child
            self.flowbox.show()
        
        # Set loading state first
        self.loading = True
        
        # Show spinner only for non-prefetch loads
        if not prefetch and hasattr(self, 'spinner') and self.spinner is not None:
            def show_spinner():
                try:
                    self.spinner.start()
                    self.spinner.show()
                except Exception as e:
                    logger.error(f"Error showing spinner: {e}")
            GLib.idle_add(show_spinner)
        
        # Store the scroll position before loading new content
        vadjustment = self.scroll.get_vadjustment()
        self.scroll_position = vadjustment.get_value() if vadjustment else 0
        
        def fetch():
            try:
                # Get the selected sort mode from the dropdown
                selected_item = self.sort_selector.get_selected_item()
                if selected_item:
                    sort_mode = selected_item.get_string().lower()
                else:
                    # Fallback to saved sort mode in config
                    sort_mode = self.config.get("sort_mode", "latest").lower()
                
                # If there's a search query and sort is set to 'popular', use 'relevance' instead
                if query and sort_mode == 'popular':
                    logger.info("Search query detected with 'popular' sort, using 'relevance' instead")
                    sort_mode = 'relevance'
                
                # Get categories and purity from config (already in correct format)
                categories = self.config.get("categories", "100")
                purity = self.config.get("purity", "100")
                
                # Ensure we have valid values
                if not isinstance(categories, str) or len(categories) != 3 or not all(c in '01' for c in categories):
                    categories = "100"
                if not isinstance(purity, str) or len(purity) != 3 or not all(p in '01' for p in purity):
                    purity = "100"
                
                # Log query information for debugging
                logger.info(f"API Request Preparation - Query parameter being used: {query}")
                
                logger.info(f"Loading wallpapers - Query: {query}, Sort: {sort_mode}, Page: {page}, Categories: {categories}, Purity: {purity}")
                
                # Get user's screen resolution
                resolution = self.get_screen_resolution()
                logger.info(f"Using resolution filter: {resolution}")
                
                # Fetch wallpapers with the specified parameters
                result = self.scraper.search_wallpapers(
                    query=query, 
                    categories=categories, 
                    purity=purity, 
                    page=page, 
                    sort=sort_mode,
                    resolution=resolution
                )
                
                # Unpack the result (wallpapers, has_next_page)
                wallpapers = result[0] if isinstance(result, tuple) and len(result) > 0 else []
                has_next_page = result[1] if isinstance(result, tuple) and len(result) > 1 else False
                
                # If we got 0 wallpapers
                if len(wallpapers) == 0:
                    if not prefetch:
                        self.spinner.stop()
                        self.spinner.hide()
                    if page == 1:
                        # If it's the first page and we got no results, try page 2
                        logger.info(f"No wallpapers found on page 1, trying page 2")
                        result = self.scraper.search_wallpapers(
                            query=query, 
                            categories=categories, 
                            purity=purity, 
                            page=2, 
                            sort=sort_mode,
                            resolution=resolution
                        )
                        wallpapers = result[0] if isinstance(result, tuple) and len(result) > 0 else []
                        has_next_page = result[1] if isinstance(result, tuple) and len(result) > 1 else False
                        
                        if len(wallpapers) > 0:
                            logger.info(f"Found {len(wallpapers)} wallpapers on page 2")
                            # Update the current page to 2 since we're showing page 2 results
                            self.current_page = 2
                            # Reset scroll position when falling back to page 2
                            if hasattr(self, 'scroll'):
                                GLib.idle_add(lambda: self.scroll.get_vadjustment().set_value(0))
                    elif page > 1 and page > self.current_page:
                        # Only show the toast if we're actually loading a new page
                        # and not just retrying the same page
                        logger.info("Reached end of search results")
                        GLib.idle_add(self.show_info_toast, "No more wallpapers found")
                
                # Handle the wallpapers based on whether this is a prefetch or not
                if prefetch:
                    # Store prefetched wallpapers with their has_next_page status
                    if not hasattr(self, 'prefetched_wallpapers'):
                        self.prefetched_wallpapers = {}
                    self.prefetched_wallpapers[page] = (wallpapers, has_next_page)
                    logger.info(f"Prefetched page {page} with {len(wallpapers)} wallpapers")
                    self.loading = False
                else:
                    # Update the current page if this is a new page load
                    if page > self.current_page:
                        self.current_page = page
                    # Update the UI with the new wallpapers
                    GLib.idle_add(self.populate_flowbox, wallpapers)
            
            except Exception as error:
                logger.error(f"Error loading wallpapers: {error}", exc_info=True)
                if not prefetch:  # Only show error for non-prefetch loads
                    error_msg = f"Failed to load wallpapers: {str(error)}"
                    GLib.idle_add(lambda: self.show_error(error_msg))
                else:
                    # Clean up prefetch state on error
                    if hasattr(self, 'prefetched_wallpapers') and page in self.prefetched_wallpapers:
                        del self.prefetched_wallpapers[page]
            finally:
                if not prefetch and hasattr(self, 'spinner'):
                    def hide_spinner():
                        try:
                            if hasattr(self, 'spinner') and self.spinner:
                                try:
                                    self.spinner.stop()
                                    self.spinner.hide()
                                except Exception as e:
                                    logger.error(f"Error stopping spinner: {e}")
                        except Exception as e:
                            logger.error(f"Error hiding spinner: {e}")
                    GLib.idle_add(hide_spinner)
                # Always reset loading state
                self.loading = False
        
        # Start the fetch operation in a background thread
        thread = threading.Thread(target=fetch, daemon=True)
        thread.start()

    def _restore_scroll_position(self):
        """Restore the scroll position after loading new items."""
        if not hasattr(self, 'scroll'):
            logger.warning("Scroll widget not available for position restoration")
            return
            
        scroll_pos = getattr(self, 'scroll_position', 0)
        adj = self.scroll.get_vadjustment()
        
        # Use a small timeout to ensure the UI has updated
        def set_scroll():
            try:
                adj.set_value(scroll_pos)
            except Exception as e:
                logger.error(f"Failed to restore scroll position: {e}")
            return False  # Don't repeat
            
        GLib.timeout_add(100, set_scroll)

    def populate_flowbox(self, wallpapers):
        """Populate the flowbox with wallpapers."""
        try:
            logger.debug(f"=== populate_flowbox called with {len(wallpapers)} wallpapers ===")
            logger.debug(f"Current page: {self.current_page}")
            logger.debug(f"Flowbox exists: {hasattr(self, 'flowbox')}")
            
            # Initialize flowbox if it doesn't exist
            if not hasattr(self, 'flowbox') or self.flowbox is None:
                logger.debug("Initializing new flowbox")
                self.flowbox = Gtk.FlowBox()
                self.flowbox.set_valign(Gtk.Align.START)
                self.flowbox.set_max_children_per_line(1000)
                self.flowbox.set_selection_mode(Gtk.SelectionMode.NONE)
                self.flowbox.set_homogeneous(True)
                self.flowbox.set_column_spacing(self.column_spacing)
                self.flowbox.set_row_spacing(self.row_spacing)
                self.flowbox.set_margin_start(self.margin)
                self.flowbox.set_margin_end(self.margin)
                self.flowbox.set_margin_top(self.margin)
                self.flowbox.set_margin_bottom(self.margin)
                
                # Add flowbox to scrollable if it's not already there
                if hasattr(self, 'scroll') and self.scroll.get_child() is None:
                    self.scroll.set_child(self.flowbox)
            
            # Clear existing wallpapers if this is the first page
            if self.current_page == 1:
                logger.debug("Clearing existing wallpapers (first page)")
                child = self.flowbox.get_first_child()
                while child is not None:
                    next_child = child.get_next_sibling()
                    self.flowbox.remove(child)
                    child = next_child
            else:
                logger.debug(f"Appending to existing wallpapers (page {self.current_page})")
                
            # Make sure flowbox is visible
            self.flowbox.show()

            # Initialize flowbox if it doesn't exist
            if not hasattr(self, 'flowbox'):
                logger.debug("Initializing new flowbox")
                self.flowbox = Gtk.FlowBox()
                self.flowbox.set_valign(Gtk.Align.START)
                self.flowbox.set_max_children_per_line(1000)
                self.flowbox.set_selection_mode(Gtk.SelectionMode.NONE)
                self.flowbox.set_homogeneous(True)
                self.flowbox.set_column_spacing(self.column_spacing)
                self.flowbox.set_row_spacing(self.row_spacing)
                self.flowbox.set_margin_start(self.margin)
                self.flowbox.set_margin_end(self.margin)
                self.flowbox.set_margin_top(self.margin)
                self.flowbox.set_margin_bottom(self.margin)
                
                self.scroll = Gtk.ScrolledWindow()
                self.scroll.set_child(self.flowbox)
                self.scroll.set_vexpand(True)
                self.main_box.append(self.scroll)
                
                vadj = self.scroll.get_vadjustment()
                vadj.connect("value-changed", self.on_scroll_changed)
            
            # Check if no wallpapers were found
            if not wallpapers:
                if self.current_page == 1:
                    toast = Adw.Toast.new("No wallpapers found with current filters")
                    toast.set_timeout(3)
                    self.toast_overlay.add_toast(toast)
                return
            
            # Store current scroll position
            adj = self.scroll.get_vadjustment()
            was_at_bottom = adj.get_value() >= adj.get_upper() - adj.get_page_size() - 10
            should_restore = hasattr(self, 'scroll_position')
            
            logger.info(f"Adding {len(wallpapers)} wallpapers to flowbox (page {self.current_page})")
            logger.debug(f"Scroll position - Value: {adj.get_value():.1f}, Upper: {adj.get_upper():.1f}, Page Size: {adj.get_page_size():.1f}")
            logger.debug(f"Was at bottom: {was_at_bottom}, Should restore: {should_restore}")
            
            # Add wallpapers to flowbox
            for wp in wallpapers:
                try:
                    # Create main container
                    box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
                    box.set_margin_top(6)
                    box.set_margin_bottom(6)
                    box.set_margin_start(6)
                    box.set_margin_end(6)
                    
                    # Create frame for the image
                    frame = Gtk.Frame()
                    frame.set_hexpand(True)
                    frame.set_vexpand(True)
                    frame.set_size_request(200, 200)
                    
                    # Create overlay for the image
                    overlay = Gtk.Overlay()
                    
                    # Create image
                    image = Gtk.Picture()
                    image.set_size_request(200, 200)
                    image.set_can_shrink(True)
                    
                    # Create a new spinner for this wallpaper
                    spinner = Gtk.Spinner()
                    spinner.set_halign(Gtk.Align.CENTER)
                    spinner.set_valign(Gtk.Align.CENTER)
                    spinner.start()
                    
                    # Add spinner to overlay
                    overlay.add_overlay(spinner)
                    overlay.set_child(image)
                    
                    # Try to load cached image first
                    if wp.is_cached():
                        try:
                            file = Gio.File.new_for_path(wp.get_cached_path())
                            image.set_file(file)
                            spinner.hide()
                        except Exception as e:
                            logger.error(f"Error loading cached image: {e}")
                            # Show error icon
                            icon_theme = Gtk.IconTheme.get_for_display(Gdk.Display.get_default())
                            icon_paintable = icon_theme.lookup_icon("image-missing-symbolic", [], 48, 1, Gtk.TextDirection.NONE, 0)
                            if icon_paintable:
                                image.set_paintable(icon_paintable)
                            spinner.hide()
                    else:
                        # Start async download
                        self.download_thumbnail_async(wp, image, spinner)
                    
                    # Add click handler
                    gesture = Gtk.GestureClick.new()
                    gesture.connect("pressed", self.on_wallpaper_clicked, wp)
                    box.add_controller(gesture)
                    
                    # Add to flowbox
                    frame.set_child(overlay)
                    box.append(frame)
                    self.flowbox.append(box)
                    
                except Exception as e:
                    logger.error(f"Error creating wallpaper widget: {e}", exc_info=True)
            
            # Force a relayout
            self.flowbox.queue_allocate()
            
            # Restore scroll position if needed
            if should_restore:
                def delayed_restore():
                    try:
                        self._restore_scroll_position()
                        if hasattr(self, 'scroll_position'):
                            delattr(self, 'scroll_position')
                    except Exception as e:
                        logger.error(f"Error in delayed_restore: {e}")
                GLib.timeout_add(300, delayed_restore)
            
        except Exception as e:
            logger.error(f"Error in populate_flowbox: {e}", exc_info=True)
            toast = Adw.Toast.new("Error loading wallpapers")
            toast.set_timeout(3)
            self.toast_overlay.add_toast(toast)

    def download_thumbnail_async(self, wp, image, spinner):
        def download_complete(path, error=None):
            if error:
                logger.error(f"Failed to download thumbnail: {error}")
                # Create a placeholder image instead of using set_icon_name
                try:
                    # Try to load a system icon as a fallback
                    icon_theme = Gtk.IconTheme.get_for_display(Gdk.Display.get_default())
                    icon_paintable = icon_theme.lookup_icon("image-missing-symbolic", [], 48, 1, Gtk.TextDirection.NONE, 0)
                    if icon_paintable:
                        image.set_paintable(icon_paintable)
                except Exception as icon_error:
                    logger.error(f"Failed to set placeholder icon: {icon_error}")
                spinner.hide()
                return
                
            try:
                file = Gio.File.new_for_path(path)
                image.set_file(file)
                spinner.hide()
            except Exception as e:
                logger.error(f"Error loading downloaded image {wp.id}: {e}")
                # Create a placeholder image instead of using set_icon_name
                try:
                    # Try to load a system icon as a fallback
                    icon_theme = Gtk.IconTheme.get_for_display(Gdk.Display.get_default())
                    icon_paintable = icon_theme.lookup_icon("image-missing-symbolic", [], 48, 1, Gtk.TextDirection.NONE, 0)
                    if icon_paintable:
                        image.set_paintable(icon_paintable)
                except Exception as icon_error:
                    logger.error(f"Failed to set placeholder icon: {icon_error}")
                spinner.hide()
        
        def download_thread():
            try:
                path = download_thumbnail(wp)
                GLib.idle_add(download_complete, path)
            except Exception as e:
                GLib.idle_add(download_complete, None, str(e))
        
        threading.Thread(target=download_thread, daemon=True).start()

    def on_scroll_changed(self, adj):
        try:
            # Get scroll values
            value = adj.get_value()
            upper = adj.get_upper()
            page_size = adj.get_page_size()
            
            # Handle header visibility based on scroll direction
            if hasattr(self, 'search_revealer'):
                SCROLL_THRESHOLD = 50  # Minimum pixels to scroll before hiding header
                
                # Determine scroll direction and distance
                scroll_delta = value - self.last_scroll_position
                self.last_scroll_position = value
                
                # Show/hide header based on scroll direction
                if scroll_delta > 5 and value > SCROLL_THRESHOLD and self.header_visible:
                    # Scrolling down, hide header and separator with fade
                    def hide_header():
                        self.search_revealer.set_opacity(0.0)
                        self.separator.set_opacity(0.0)
                        self.search_revealer.set_reveal_child(False)
                        self.separator.set_visible(False)
                        self.search_container.set_visible(False)  # Hide the entire container
                        self.header_visible = False
                        return False  # Don't repeat
                    
                    # Start hiding after a short delay
                    GLib.timeout_add(50, hide_header)
                    
                elif scroll_delta < -5 and not self.header_visible:
                    # Scrolling up, show header and separator with fade
                    self.search_container.set_visible(True)  # Show the container first
                    self.search_revealer.set_reveal_child(True)
                    self.separator.set_visible(True)
                    self.header_visible = True
                    
                    # Fade in the header and separator
                    self.search_revealer.set_opacity(0.0)
                    self.separator.set_opacity(0.0)
                    self.search_revealer.set_opacity(1.0)
                    self.separator.set_opacity(1.0)
            
            # Calculate scroll position (0-1)
            scroll_position = (value + page_size) / upper if upper > 0 else 0
            next_page = self.current_page + 1
            
            # Initialize prefetched_wallpapers if it doesn't exist
            if not hasattr(self, 'prefetched_wallpapers'):
                self.prefetched_wallpapers = {}
            
            # Detailed debug logging
            logger.debug("\n--- Scroll Event ---")
            logger.debug(f"Value: {value:.1f}, Upper: {upper:.1f}, Page Size: {page_size:.1f}")
            logger.debug(f"Scroll Position: {scroll_position:.2f} ({(scroll_position*100):.0f}%)")
            logger.debug(f"Current Page: {self.current_page}, Next Page: {next_page}")
            logger.debug(f"Loading: {self.loading}, Prefetched Pages: {list(self.prefetched_wallpapers.keys())}")
            
            if self.loading:
                logger.debug("Skipping scroll check - already loading")
                return
            
            # Handle case where content is smaller than viewport
            if upper <= page_size:
                logger.debug("Content smaller than viewport, skipping scroll check")
                return
            
            # Calculate scroll position as a percentage (0 to 1)
            scroll_ratio = (value + page_size) / upper if upper > 0 else 0
            
            # Only proceed if we're at least 10% scrolled (to avoid triggering on initial load)
            if scroll_ratio > 0.1:
                next_page = self.current_page + 1
                
                # Initialize prefetched_wallpapers if it doesn't exist
                if not hasattr(self, 'prefetched_wallpapers'):
                    self.prefetched_wallpapers = {}
                
                # Detailed debug logging
                logger.debug("\n--- Scroll Event ---")
                logger.debug(f"Value: {value:.1f}, Upper: {upper:.1f}, Page Size: {page_size:.1f}")
                logger.debug(f"Scroll Position: {scroll_ratio:.2f} ({scroll_ratio*100:.0f}%)")
                logger.debug(f"Current Page: {self.current_page}, Next Page: {next_page}")
                logger.debug(f"Loading: {self.loading}, Prefetched Pages: {list(self.prefetched_wallpapers.keys())}")
                
                if self.loading:
                    logger.debug("Skipping scroll check - already loading")
                    return
                    
                # Check if we're near the bottom (within 30% of the page size from the bottom)
                scroll_threshold = page_size * 0.3  # 30% of the page size
                if value + page_size < upper - scroll_threshold:
                    logger.debug(f"Not at threshold yet: {value + page_size:.1f} < {upper - scroll_threshold:.1f}")
                    return
                
                if next_page in self.prefetched_wallpapers:
                    try:
                        # Get the prefetched wallpapers and has_next_page status
                        prefetched_data = self.prefetched_wallpapers.pop(next_page)
                        if isinstance(prefetched_data, tuple) and len(prefetched_data) == 2:
                            wallpapers, _ = prefetched_data
                        else:
                            # Handle case where we only have wallpapers (backward compatibility)
                            wallpapers = prefetched_data if isinstance(prefetched_data, list) else []
                        
                        logger.info(f"Loading prefetched page {next_page} with {len(wallpapers)} wallpapers")
                        self.current_page = next_page
                        
                        # Store the has_next_page value for use in the callback
                        current_has_next_page = has_next_page
                        
                        # Update UI on the main thread
                        def update_ui():
                            try:
                                self.populate_flowbox(wallpapers)
                                # Prefetch the next page
                                next_next_page = next_page + 1
                                if next_next_page not in self.prefetched_wallpapers and current_has_next_page:
                                    logger.info(f"Prefetching next page {next_next_page}")
                                    self.load_wallpapers(
                                        query=self.current_query, 
                                        page=next_next_page, 
                                        prefetch=True
                                    )
                            except Exception as e:
                                logger.error(f"Error in update_ui: {e}")
                        
                        GLib.idle_add(update_ui)
                    except Exception as e:
                        logger.error(f"Error processing prefetched page {next_page}: {e}")
                        # Fall back to normal loading if there's an error with the prefetched data
                        if not self.loading:
                            logger.info(f"Falling back to normal loading for page {next_page}")
                            self.load_wallpapers(
                                query=self.current_query, 
                                page=next_page
                            )
                elif not self.loading:
                    # No prefetch available, load normally
                    logger.info(f"Loading page {next_page} (no prefetch available)")
                    self.load_wallpapers(
                        query=self.current_query, 
                        page=next_page
                    )
            
            # Prefetch next page when 50% scrolled
            elif scroll_position > 0.5 and next_page not in self.prefetched_wallpapers and not self.loading:
                logger.info(f"Prefetching page {next_page} at {scroll_position:.2f} scroll position")
                self.load_wallpapers(
                    query=self.current_query, 
                    page=next_page, 
                    prefetch=True
                )
                
        except Exception as e:
            logger.error(f"Error in on_scroll_changed: {e}", exc_info=True)

    def apply_preferences(self, reload_wallpapers=True):
        """Apply the current preferences and optionally reload wallpapers.
        
        Args:
            reload_wallpapers: If True, reload wallpapers with new preferences
        """
        try:
            # Save the current preferences
            if not self.save_preferences():
                logger.error("Failed to save preferences")
                return False
                
            # If we need to reload wallpapers with new preferences
            if reload_wallpapers:
                # Reset to first page
                self.current_page = 1
                
                # Clear the flowbox
                child = self.flowbox.get_first_child()
                while child is not None:
                    next_child = child.get_next_sibling()
                    self.flowbox.remove(child)
                    child = next_child
                
                # Reset scroll position
                self._reset_scroll_position()
                
                # Reload wallpapers with current query and new preferences
                self.load_wallpapers(query=self.current_query, page=1, force_reload=True)
                
                logger.info("Preferences applied and wallpapers reloaded")
                
            return True
            
        except Exception as e:
            logger.error(f"Error applying preferences: {e}")
            return False
    
    def on_preferences(self, action, param):
        """Show the preferences dialog."""
        logger.info("Opening preferences dialog")
        
        # Get current settings with proper defaults
        self.config.setdefault("selected_categories", ["General"])
        self.config.setdefault("selected_purity", ["SFW"])
        
        current_categories = self.config["selected_categories"]
        current_purity = self.config["selected_purity"]
        
        # Ensure SFW is always selected if no purity is selected
        if not current_purity:
            current_purity = ["SFW"]
            self.config["selected_purity"] = current_purity
        
        # Create the preferences window
        dialog = Adw.PreferencesWindow()
        dialog.set_title("Preferences")
        dialog.set_modal(True)
        dialog.set_transient_for(self.props.active_window)
        dialog.set_default_size(250, 450)
        
        # Initialize switches dictionaries if they don't exist
        if not hasattr(self, 'category_switches'):
            self.category_switches = {}
        if not hasattr(self, 'purity_switches'):
            self.purity_switches = {}

        # Create the Categories page
        categories_page = Adw.PreferencesPage(title="Categories", icon_name="view-grid-symbolic")
        dialog.add(categories_page)
        
        # Categories group
        categories_group = Adw.PreferencesGroup(
            title="Wallpaper Categories",
            description="Select the types of wallpapers you want to see"
        )
        categories_page.add(categories_group)
        
        # Category switches with descriptions
        category_options = [
            ("General", "General wallpapers"),
            ("Anime", "Anime/Manga style"),
            ("People", "Wallpapers featuring people")
        ]
        
        for cat, subtitle in category_options:
            row = Adw.SwitchRow(title=cat, subtitle=subtitle)
            row.set_active(cat in current_categories)
            self.category_switches[cat.lower()] = row
            categories_group.add(row)
        
        # Create the Content Safety page
        safety_page = Adw.PreferencesPage(title="Content Safety", icon_name="security-high-symbolic")
        dialog.add(safety_page)
        
        # Purity group
        purity_group = Adw.PreferencesGroup(
            title="Purity Levels",
            description="Filter content based on purity level"
        )
        safety_page.add(purity_group)
        
        # SFW toggle (optional)
        sfw_row = Adw.SwitchRow(
            title="SFW (Safe For Work)",
            subtitle="Suitable for all audiences"
        )
        sfw_row.set_active("SFW" in current_purity or not current_purity)  # Default to enabled if empty
        self.purity_switches["sfw"] = sfw_row
        purity_group.add(sfw_row)
        
        # Sketchy toggle (optional)
        sketchy_row = Adw.SwitchRow(
            title="Sketchy (NSFW)",
            subtitle="Mature Content (Not Safe For Work)"
        )
        sketchy_row.set_active("Sketchy" in current_purity)
        self.purity_switches["sketchy"] = sketchy_row
        purity_group.add(sketchy_row)
        
        # Add note about content safety
        note_group = Adw.PreferencesGroup()
        note_row = Adw.ActionRow(
            title="Note",
            subtitle="With Sketchy disabled, only SFW (Safe For Work) content will be shown"
        )
        note_group.add(note_row)
        safety_page.add(note_group)
        
        # Connect to the close-request signal to save preferences when window is closed
        dialog.connect("close-request", self.on_preferences_close)
        dialog.present()

    def on_preferences_close(self, dialog):
        """Handle the preferences window close event."""
        # Get selected categories
        selected_categories = []
        for cat, switch in getattr(self, 'category_switches', {}).items():
            if switch.get_active():
                selected_categories.append(cat.capitalize())
        
        # Get selected purity levels (both optional, default to SFW if none selected)
        sfw_enabled = self.purity_switches.get("sfw", None) and self.purity_switches["sfw"].get_active()
        sketchy_enabled = self.purity_switches.get("sketchy", None) and self.purity_switches["sketchy"].get_active()
        
        # If neither is selected, default to SFW
        if not sfw_enabled and not sketchy_enabled:
            sfw_enabled = True
        
        # Ensure at least one category is selected
        if not selected_categories:
            selected_categories = ["General"]
        
        # Convert to the format expected by the API
        categories_str = ""
        for cat in ["General", "Anime", "People"]:
            categories_str += "1" if cat in selected_categories else "0"
        
        # Generate purity string (SFW and Sketchy are optional, NSFW is always 0)
        purity_str = f"{1 if sfw_enabled else 0}{1 if sketchy_enabled else 0}0"
        
        # Get current categories/purity for comparison
        current_categories = self.config.get("selected_categories", [])
        current_purity = self.config.get("selected_purity", [])
        
        # Get current settings for comparison
        current_sfw_enabled = "SFW" in self.config.get("selected_purity", ["SFW"])
        current_sketchy_enabled = "Sketchy" in self.config.get("selected_purity", [])
        
        # Only update if there are changes
        if (set(selected_categories) != set(current_categories) or 
            sfw_enabled != current_sfw_enabled or 
            sketchy_enabled != current_sketchy_enabled):
            
            # Update config with new values
            self.config["selected_categories"] = selected_categories
            selected_purity = []
            if sfw_enabled:
                selected_purity.append("SFW")
            if sketchy_enabled:
                selected_purity.append("Sketchy")
            self.config["selected_purity"] = selected_purity if selected_purity else ["SFW"]  # Default to SFW if empty
            self.config["categories"] = categories_str
            self.config["purity"] = purity_str
            
            # Save and apply preferences
            if self.save_preferences():
                logger.info(f"Updated preferences - Categories: {categories_str}, Purity: {purity_str}")
                
                # Apply preferences and reload wallpapers with new settings
                if not self.apply_preferences(reload_wallpapers=True):
                    self.show_error("Failed to apply preferences")
                
                # Show success toast
                toast = Adw.Toast.new("Preferences saved and wallpapers refreshed")
                self.toast_overlay.add_toast(toast)
            else:
                self.show_error("Failed to save preferences")
        
        dialog.destroy()
        
    def get_wallpapers_dir(self):
        """Get the wallpapers directory in user's Pictures folder"""
        # Get user's home directory
        home_dir = os.path.expanduser("~")
        # Create path to Pictures/WSelector folder
        return os.path.join(home_dir, "Pictures", "WSelector")
        
    def get_screen_resolution(self):
        """Get the user's screen resolution"""
        try:
            # Get the primary monitor
            display = Gdk.Display.get_default()
            if display:
                monitor = display.get_monitors().get_item(0)
                if monitor:
                    geometry = monitor.get_geometry()
                    width = geometry.width
                    height = geometry.height
                    logger.info(f"Detected screen resolution: {width}x{height}")
                    return f"{width}x{height}"
        except Exception as e:
            logger.error(f"Error detecting screen resolution: {e}")
        
        # Return None if we couldn't detect the resolution
        return None
        
    def on_wallpaper_clicked(self, gesture, n_press, x, y, wallpaper):
        """Handle wallpaper click"""
        logger.info(f"Wallpaper clicked: {wallpaper.id}")
        
        # Check if the wallpaper is already downloaded
        wallpaper_path = os.path.join(self.get_wallpapers_dir(), f"{wallpaper.id}.jpg")
        
        if os.path.exists(wallpaper_path):
            # If already downloaded, show the preview
            try:
                # Get all downloaded wallpapers for navigation
                wallpaper_dir = self.get_wallpapers_dir()
                image_extensions = ('.jpg', '.jpeg', '.png', '.webp')
                wallpaper_files = sorted([
                    os.path.join(wallpaper_dir, f) for f in os.listdir(wallpaper_dir)
                    if os.path.isfile(os.path.join(wallpaper_dir, f)) and f.lower().endswith(image_extensions)
                ], key=os.path.getmtime, reverse=True)
                
                # Find current index
                try:
                    current_index = wallpaper_files.index(wallpaper_path)
                except ValueError:
                    current_index = 0
                
                # Show preview with navigation
                self._show_wallpaper_preview(
                    wallpaper_path,
                    self.props.active_window,
                    wallpaper_files,
                    current_index
                )
                return
                
            except Exception as e:
                logger.error(f"Error showing preview: {e}")
                self.show_error_toast("Error showing preview")
        else:
            # If not downloaded, show download dialog
            dialog = Adw.MessageDialog.new(
                self.props.active_window,
                "Download Wallpaper",
                f"Do you want to download this wallpaper?\n\nID: {wallpaper.id}"
            )
            
            dialog.add_response("cancel", "Cancel")
            dialog.add_response("download", "Download")
            dialog.set_response_appearance("download", Adw.ResponseAppearance.SUGGESTED)
            dialog.connect("response", self.on_download_response, wallpaper)
            dialog.present()
    
    def on_download_response(self, dialog, response, wallpaper):
        if response == "download":
            logger.info(f"Downloading wallpaper: {wallpaper.id}")
            # Show a toast notification that download is starting
            self.show_info_toast(f"Starting download: {wallpaper.id}")
            # Start the download in a separate thread to keep the UI responsive
            threading.Thread(target=self.download_wallpaper, args=(wallpaper,), daemon=True).start()
        dialog.destroy()
    
    def download_wallpaper(self, wallpaper):
        try:
            # Create downloads directory if it doesn't exist
            download_dir = self.get_wallpapers_dir()
            if not os.path.exists(download_dir):
                os.makedirs(download_dir)
                
            # Get the filename from the ID
            filename = f"{wallpaper.id}.jpg"
            filepath = os.path.join(download_dir, filename)
            
            # Download the wallpaper
            # The url field in WallpaperInfo contains the direct image URL from the API
            logger.info(f"Downloading from URL: {wallpaper.url}")
            
            # Create a request with a User-Agent header to avoid 403 errors
            headers = {
                'User-Agent': 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36'
            }
            req = urllib.request.Request(wallpaper.url, headers=headers)
            with urllib.request.urlopen(req) as response, open(filepath, 'wb') as out_file:
                out_file.write(response.read())
            
            # Show success notification
            GLib.idle_add(self.show_download_toast, filepath)
            logger.info(f"Wallpaper downloaded to: {filepath}")
        except Exception as e:
            logger.error(f"Error downloading wallpaper: {e}")
            GLib.idle_add(self.show_error_toast, f"Error downloading wallpaper: {str(e)}")
    
    def show_download_toast(self, filepath):
        toast = Adw.Toast.new(f"Wallpaper downloaded")
        toast.set_button_label("Show Location")
        
        # Show a dialog with the folder path and options
        def show_location_dialog(toast):
            logger.info("=== Show Location button clicked ===")
            try:
                folder_path = os.path.dirname(filepath) if os.path.isfile(filepath) else filepath
                logger.info(f"Folder path: {folder_path}")
                logger.info(f"Path exists: {os.path.exists(folder_path)}")
                logger.info(f"Is directory: {os.path.isdir(folder_path)}")
                
                # Create a custom dialog window instead of using Adw.MessageDialog
                # This gives us more control over the button order
                window = Adw.Window.new()
                window.set_title("Wallpaper Location")
                window.set_default_size(400, -1)
                window.set_transient_for(self.props.active_window)
                window.set_modal(True)
                
                # Create a vertical box for the content
                content_box = Gtk.Box.new(Gtk.Orientation.VERTICAL, 10)
                content_box.set_margin_top(24)
                content_box.set_margin_bottom(24)
                content_box.set_margin_start(24)
                content_box.set_margin_end(24)
                
                # Add a header
                header = Adw.StatusPage.new()
                header.set_title("Wallpaper Location")
                header.set_description(f"Your wallpapers are saved in:\n{folder_path}")
                header.set_icon_name("folder-pictures-symbolic")
                content_box.append(header)
                
                # Create a box for buttons
                button_box = Gtk.Box.new(Gtk.Orientation.VERTICAL, 10)
                button_box.set_margin_top(12)
                
                # Create buttons in the desired order
                if os.path.isfile(filepath):
                    # View button
                    view_button = Gtk.Button.new_with_label("Preview Wallpaper")
                    view_button.add_css_class("suggested-action")
                    view_button.connect("clicked", lambda b: self._handle_location_action(window, "view", filepath, folder_path))
                    button_box.append(view_button)
                    
                    # Set as wallpaper button
                    wallpaper_button = Gtk.Button.new_with_label("Set as Background")
                    wallpaper_button.connect("clicked", lambda b: self._handle_location_action(window, "wallpaper", filepath, folder_path))
                    button_box.append(wallpaper_button)
                
                # Browse downloads button
                browse_button = Gtk.Button.new_with_label("Browse Downloads")
                browse_button.connect("clicked", lambda b: self._handle_location_action(window, "browse", filepath, folder_path))
                button_box.append(browse_button)
                
                # Copy path button
                copy_button = Gtk.Button.new_with_label("Copy Path")
                copy_button.connect("clicked", lambda b: self._handle_location_action(window, "copy", filepath, folder_path))
                button_box.append(copy_button)
                
                # Close button at the bottom
                close_button = Gtk.Button.new_with_label("Close")
                close_button.get_style_context().add_class("destructive-action")
                close_button.connect("clicked", lambda b: window.destroy())
                close_button.add_css_class("flat")
                close_button.set_margin_top(6)
                button_box.append(close_button)
                
                # Add the button box to the content
                content_box.append(button_box)
                
                # Set the content and show the window
                window.set_content(content_box)
                window.present()
                
            except Exception as e:
                logger.error(f"Error showing location dialog: {e}")
                logger.error(f"Error type: {type(e).__name__}")
                import traceback
                logger.error(f"Traceback: {traceback.format_exc()}")
                self.show_error_toast(f"Error: {e}")
            logger.info("=== End of Show Location handling ===")
        
        # Connect the button click to the dialog function
        toast.connect("button-clicked", lambda t: show_location_dialog(t))
        toast.set_timeout(5)
        self.toast_overlay.add_toast(toast)
        return False
        
    def _handle_location_action(self, window, action, filepath, folder_path):
        """Handle actions from the location dialog buttons"""
        try:
            if action == "copy":
                # Copy the path to clipboard
                clipboard = Gdk.Display.get_default().get_clipboard()
                clipboard.set(folder_path)
                
                # Show confirmation toast
                copy_toast = Adw.Toast.new("Path copied to clipboard")
                copy_toast.set_timeout(2)
                self.toast_overlay.add_toast(copy_toast)
                
                # Keep the dialog open
                return True
                
            elif action == "view" and os.path.isfile(filepath):
                # Close the dialog
                window.destroy()
                
                # Open the image in a new window
                self._show_wallpaper_preview(filepath, self.props.active_window)
                return True
                
            elif action == "wallpaper" and os.path.isfile(filepath):
                # Close the dialog
                window.destroy()
                
                # Set as wallpaper
                self._set_as_background(filepath)
                return True
                
            elif action == "browse":
                # Close the dialog
                window.destroy()
                
                # Show a file browser dialog for the downloaded wallpapers
                # Force refresh to show newly downloaded wallpapers
                self._show_downloads_browser(force_refresh=True)
                return True
                
            return False
            
        except Exception as e:
            logger.error(f"Error handling location action: {e}")
            self.show_error_toast(f"Error: {e}")
            return False
        
    def _show_wallpaper_preview(self, filepath, parent_window=None, wallpaper_list=None, current_index=0):
        """Show a preview of the wallpaper in a new window with navigation
        
        Args:
            filepath: Path to the current wallpaper file to preview
            parent_window: Optional parent window to set as transient parent
            wallpaper_list: Optional list of wallpaper filepaths for navigation
            current_index: Current index in the wallpaper_list
        """
        logger.info("Creating preview window...")
        
        # If no parent window is provided, try to get the active window
        if parent_window is None:
            parent_window = self.props.active_window
            
        # If no wallpaper list is provided, get all wallpapers from the downloads directory
        if wallpaper_list is None:
            wallpapers_dir = self.get_wallpapers_dir()
            try:
                wallpaper_list = sorted([
                    os.path.join(wallpapers_dir, f) 
                    for f in os.listdir(wallpapers_dir) 
                    if f.lower().endswith(('.png', '.jpg', '.jpeg', '.webp'))
                ])
                # Find the current file in the list
                try:
                    current_index = wallpaper_list.index(os.path.abspath(filepath))
                except ValueError:
                    current_index = 0
                    wallpaper_list.insert(0, os.path.abspath(filepath))
            except Exception as e:
                logger.error(f"Error getting wallpaper list: {e}")
                wallpaper_list = [os.path.abspath(filepath)]
                current_index = 0
        
        # Create the preview window
        preview_window = Gtk.Window()
        preview_window.set_title(f"Wallpaper Preview ({current_index + 1}/{len(wallpaper_list)})")
        preview_window.set_default_size(1000, 700)
        preview_window.set_modal(True)
        if parent_window:
            preview_window.set_transient_for(parent_window)
        preview_window.set_destroy_with_parent(True)
        
        # Ensure the window is properly shown
        preview_window.set_visible(True)
        
        # Store the wallpaper list and current index in the window object
        preview_window.wallpaper_list = wallpaper_list
        preview_window.current_index = current_index
        logger.info(f"Preview window created with {len(wallpaper_list)} wallpapers, starting at index {current_index}")
        
        # Main vertical box
        main_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        main_box.set_margin_top(10)
        main_box.set_margin_bottom(10)
        main_box.set_margin_start(10)
        main_box.set_margin_end(10)
        preview_window.set_child(main_box)
        
        # Create a scrolled window for the image
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_hexpand(True)
        scrolled.set_vexpand(True)
        scrolled.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        main_box.append(scrolled)
        
        # Create a viewport to handle the image
        viewport = Gtk.Viewport()
        scrolled.set_child(viewport)
        
        # Create a box to center the image
        center_box = Gtk.Box(halign=Gtk.Align.CENTER, valign=Gtk.Align.CENTER)
        viewport.set_child(center_box)
        
        # Create the picture widget for displaying the wallpaper
        preview_window.preview_image = Gtk.Picture()
        preview_window.preview_image.set_can_shrink(False)  # Allow the image to be larger than the viewport
        preview_window.preview_image.set_size_request(-1, -1)  # Let the image determine its size
        
        # Make the picture widget expand to fill available space
        preview_window.preview_image.set_hexpand(True)
        preview_window.preview_image.set_vexpand(True)
        
        # Enable smooth scrolling
        scrolled.set_kinetic_scrolling(True)
        scrolled.set_propagate_natural_width(True)
        scrolled.set_propagate_natural_height(True)
        
        # Add the image to the center box
        center_box.append(preview_window.preview_image)
        
        # Set up drag-to-pan behavior
        drag = Gtk.GestureDrag.new()
        drag.set_button(1)  # Left mouse button
        preview_window.preview_image.add_controller(drag)
        
        # Connect the drag signals
        drag.connect("drag-begin", self._on_drag_begin, scrolled)
        drag.connect("drag-update", self._on_drag_update, scrolled)
        drag.connect("drag-end", self._on_drag_end, scrolled)
        
        # Button box at the bottom
        button_box = Gtk.Box(spacing=12, margin_top=6, margin_bottom=6, margin_start=6, margin_end=6)
        button_box.set_halign(Gtk.Align.CENTER)
        main_box.append(button_box)
        
        # Main container for all buttons
        buttons_container = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12, halign=Gtk.Align.CENTER)
        button_box.append(buttons_container)
        
        # Navigation box for Previous/Set as Background/Next
        nav_box = Gtk.Box(spacing=12, halign=Gtk.Align.CENTER)
        buttons_container.append(nav_box)
        
        # Previous button
        prev_button = Gtk.Button(icon_name="go-previous-symbolic")
        prev_button.set_tooltip_text("Previous")
        prev_button.set_size_request(48, 48)
        prev_button.set_hexpand(False)
        prev_button.connect("clicked", self._on_prev_wallpaper, preview_window)
        nav_box.append(prev_button)
        preview_window.prev_button = prev_button
        
        # Set as Background button (centered between navigation buttons)
        set_bg_button = Gtk.Button(label="Set as Background")
        set_bg_button.get_style_context().add_class("suggested-action")
        set_bg_button.set_margin_start(12)
        set_bg_button.set_margin_end(12)
        set_bg_button.set_size_request(180, 48)  # Wider button for better touch targets
        set_bg_button.connect("clicked", 
            lambda *_: self._set_as_background(wallpaper_list[preview_window.current_index], preview_window))
        nav_box.append(set_bg_button)
        preview_window.set_bg_button = set_bg_button
        
        # Next button
        next_button = Gtk.Button(icon_name="go-next-symbolic")
        next_button.set_tooltip_text("Next")
        next_button.set_size_request(48, 48)
        next_button.set_hexpand(False)
        next_button.connect("clicked", self._on_next_wallpaper, preview_window)
        nav_box.append(next_button)
        preview_window.next_button = next_button
        
        # Close button in its own row
        close_button = Gtk.Button(label="Close")
        close_button.set_size_request(120, 40)
        close_button.connect("clicked", lambda *_: preview_window.close())
        buttons_container.append(close_button)
        
        # Set the initial image
        self._update_preview_image(preview_window, current_index)
        
        # Show the window
        preview_window.present()
        logger.info("Preview window shown with buttons")
        
        return preview_window
        
    def _on_prev_wallpaper(self, button, preview_window):
        """Handle clicking the previous button in the preview window"""
        logger.info(f"Previous button clicked")
        
        wallpaper_list = preview_window.wallpaper_list
        if not wallpaper_list:
            logger.warning("No wallpapers in the list")
            return
            
        current_index = preview_window.current_index
        new_index = (current_index - 1) % len(wallpaper_list)  # Wrap around to the end if at start
        
        logger.info(f"Current index: {current_index}, New index: {new_index}")
        logger.info(f"Loading wallpaper: {wallpaper_list[new_index]}")
        
        self._update_preview_image(preview_window, new_index)
    
    def _on_next_wallpaper(self, button, preview_window):
        """Handle clicking the next button in the preview window"""
        logger.info(f"Next button clicked")
        
        wallpaper_list = preview_window.wallpaper_list
        if not wallpaper_list:
            logger.warning("No wallpapers in the list")
            return
            
        current_index = preview_window.current_index
        new_index = (current_index + 1) % len(wallpaper_list)  # Wrap around to the start if at end
        
        logger.info(f"Current index: {current_index}, New index: {new_index}")
        logger.info(f"Loading wallpaper: {wallpaper_list[new_index]}")
        
        self._update_preview_image(preview_window, new_index)
    
    def _update_preview_image(self, preview_window, new_index):
        """Update the preview window with a different wallpaper"""
        logger.info(f"Updating preview image to index {new_index}")
        logger.info(f"Wallpaper list length: {len(preview_window.wallpaper_list)}")
        
        # Update the current index and window title
        preview_window.current_index = new_index
        preview_window.set_title(f"Wallpaper Preview ({new_index + 1}/{len(preview_window.wallpaper_list)})")
        
        # Update the set as background button to use the current wallpaper
        if hasattr(preview_window, 'set_bg_button'):
            # Disconnect any existing handlers safely
            try:
                preview_window.set_bg_button.disconnect_by_func(self._set_as_background)
            except (TypeError, AttributeError):
                # No existing connection, or disconnect failed
                pass
                
            # Connect new handler
            preview_window.set_bg_button.connect("clicked", 
                lambda *_: self._set_as_background(
                    preview_window.wallpaper_list[new_index], 
                    preview_window
                )
            )
        
        # Get the current wallpaper path
        wallpaper_list = preview_window.wallpaper_list
        current_wallpaper = wallpaper_list[new_index]
        logger.info(f"Setting new wallpaper: {current_wallpaper}")
        
        try:
            # Load the new image
            pixbuf = GdkPixbuf.Pixbuf.new_from_file(current_wallpaper)
            width = pixbuf.get_width()
            height = pixbuf.get_height()
            
            # Calculate maximum dimensions while maintaining aspect ratio
            max_width = 1600
            max_height = 900
            
            if width > max_width or height > max_height:
                ratio = min(max_width / width, max_height / height)
                new_width = int(width * ratio)
                new_height = int(height * ratio)
                pixbuf = pixbuf.scale_simple(
                    new_width,
                    new_height,
                    GdkPixbuf.InterpType.BILINEAR
                )
            
            # Create a texture from the scaled pixbuf
            texture = Gdk.Texture.new_for_pixbuf(pixbuf)
            
            # Update the picture widget
            preview_window.preview_image.set_paintable(texture)
            preview_window.preview_image.set_size_request(pixbuf.get_width(), pixbuf.get_height())
            
            # Update the window title with current position
            preview_window.set_title(f"Wallpaper Preview ({new_index + 1}/{len(wallpaper_list)})")
            
            # In GTK4, we don't need to manually process events
            # The UI will update automatically when we set the paintable
            pass
                
        except Exception as e:
            logger.error(f"Error updating preview image: {e}")
            self.show_error_toast(f"Failed to load image: {str(e)}")
    
    def _on_drag_begin(self, gesture, start_x, start_y, scrolled):
        """Handle drag begin event"""
        # Skip cursor handling on Wayland as it's not needed
        return True
    
    def _on_drag_update(self, gesture, offset_x, offset_y, scrolled):
        """Handle drag update events"""
        # Get the current adjustment values
        hadj = scrolled.get_hadjustment()
        vadj = scrolled.get_vadjustment()
        
        # Calculate new positions with bounds checking
        new_hadj = max(0, min(hadj.get_value() - offset_x, 
                           hadj.get_upper() - hadj.get_page_size()))
        new_vadj = max(0, min(vadj.get_value() - offset_y,
                           vadj.get_upper() - vadj.get_page_size()))
        
        # Update the adjustments
        hadj.set_value(new_hadj)
        vadj.set_value(new_vadj)
        
        return True
    
    def _on_drag_end(self, gesture, offset_x, offset_y, scrolled):
        """Handle drag end event"""
        # No need to reset cursor on Wayland
        return True

    def _update_nav_buttons(self, preview_window):
        """Update the navigation buttons' sensitivity"""
        try:
            current_index = preview_window.current_index
            wallpaper_list = preview_window.wallpaper_list
            
            # Update button sensitivity
            preview_window.prev_button.set_sensitive(current_index > 0)
            preview_window.next_button.set_sensitive(current_index < len(wallpaper_list) - 1)
            
            # Reset cursor when changing images
            if hasattr(preview_window, 'preview_image'):
                preview_window.preview_image.set_cursor_from_name("grab")
            
            # Force the buttons to update their state
            preview_window.prev_button.queue_draw()
            preview_window.next_button.queue_draw()
            
            # Force a UI update
            while Gtk.events_pending():
                Gtk.main_iteration()
                
        except Exception as e:
            logger.exception(f"Error updating navigation buttons: {str(e)}")
    
    def _set_wallpaper_thread(self, filepath):
        """Thread function to set the wallpaper without blocking the UI.
        
        Args:
            filepath (str): Path to the wallpaper image file
        """
        try:
            logger.info(f"Attempting to set wallpaper: {filepath}")
            
            # First try the xdg-desktop-portal method
            if os.path.exists('/.flatpak-info'):
                logger.info("Running in Flatpak environment")
                
                # Convert to URI format
                file_uri = f"file://{filepath}"
                
                # Try GNOME method first (most common for Flatpak)
                try:
                    # Set wallpaper for light mode
                    result = subprocess.run(
                        ["flatpak-spawn", "--host", "gsettings", "set", 
                         "org.gnome.desktop.background", "picture-uri", f"'{file_uri}'"],
                        capture_output=True,
                        text=True,
                        timeout=10
                    )
                    
                    if result.returncode == 0:
                        # Set for dark mode if supported (GNOME 42+)
                        subprocess.run(
                            ["flatpak-spawn", "--host", "gsettings", "set", 
                             "org.gnome.desktop.background", "picture-uri-dark", f"'{file_uri}'"],
                            capture_output=True
                        )
                        
                        # Set the picture options (zoom, centered, scaled, etc.)
                        subprocess.run(
                            ["flatpak-spawn", "--host", "gsettings", "set", 
                             "org.gnome.desktop.background", "picture-options", "'zoom'"],
                            capture_output=True
                        )
                        
                        logger.info("Successfully set wallpaper using GNOME method")
                        GLib.idle_add(lambda: self.show_success_toast("Wallpaper set successfully!"))
                        return
                        
                except Exception as e:
                    logger.warning(f"GNOME method failed: {e}")
                
                # Fallback to xdg-desktop-portal method
                try:
                    logger.info("Trying xdg-desktop-portal method")
                    result = subprocess.run([
                        'dbus-send', '--session',
                        '--print-reply=literal',
                        '--dest=org.freedesktop.portal.Desktop',
                        '/org/freedesktop/portal/desktop',
                        'org.freedesktop.portal.Background.SetBackground',
                        f'string:{file_uri}'
                    ], capture_output=True, text=True, timeout=10)
                    
                    logger.info(f"Portal call result: {result.returncode}")
                    if result.returncode == 0:
                        GLib.idle_add(lambda: self.show_success_toast("Wallpaper set successfully!"))
                        return
                    
                    if result.stderr:
                        logger.error(f"Portal error: {result.stderr}")
                        
                except Exception as e:
                    logger.error(f"Portal method failed: {e}")
            
            # If we're here, Flatpak methods failed or we're not in Flatpak
            # Try to detect desktop environment and use appropriate method
            try:
                desktop_env = self._detect_desktop_environment()
                logger.info(f"Detected desktop environment: {desktop_env}")
                
                if "gnome" in desktop_env or "ubuntu" in desktop_env or "pop" in desktop_env:
                    # GNOME/Unity/Cinnamon/MATE/Budgie
                    subprocess.run(["gsettings", "set", "org.gnome.desktop.background", "picture-uri", f"file://{filepath}"])
                    subprocess.run(["gsettings", "set", "org.gnome.desktop.background", "picture-uri-dark", f"file://{filepath}"])
                    subprocess.run(["gsettings", "set", "org.gnome.desktop.background", "picture-options", "zoom"])
                elif "kde" in desktop_env or "plasma" in desktop_env:
                    # KDE Plasma
                    subprocess.run(["plasma-apply-wallpaperimage", filepath])
                elif "xfce" in desktop_env:
                    # XFCE
                    subprocess.run(["xfconf-query", "-c", "xfce4-desktop", "-p", 
                                  "/backdrop/screen0/monitor0/workspace0/last-image", "-s", filepath])
                else:
                    # Try feh as a last resort (common in minimal WMs)
                    subprocess.run(["feh", "--bg-fill", filepath])
                
                logger.info(f"Wallpaper set using {desktop_env} method")
                GLib.idle_add(lambda: self.show_success_toast("Wallpaper set successfully!"))
                return
                
            except Exception as e:
                logger.error(f"Error setting wallpaper: {e}")
                raise Exception("Could not set wallpaper automatically")
            
        except Exception as error:
            logger.error(f"Error in wallpaper thread: {error}", exc_info=True)
            GLib.idle_add(lambda: self.show_error_toast("Could not set wallpaper automatically"))
            GLib.idle_add(lambda: self._show_wallpaper_instructions_dialog(filepath))
        finally:
            # Reset the flag when done
            self._is_setting_wallpaper = False
    
    def _set_as_background(self, filepath, parent_window=None):
        """Set the wallpaper as the desktop background
        
        Args:
            filepath (str): Path to the wallpaper image file
            parent_window: Optional parent window for dialogs
            
        Returns:
            bool: True if the wallpaper setting process was started successfully, False otherwise
        """
        # Use a flag to prevent multiple simultaneous executions
        if hasattr(self, '_is_setting_wallpaper') and self._is_setting_wallpaper:
            logger.info("Wallpaper setting already in progress, skipping duplicate request")
            return False
            
        try:
            self._is_setting_wallpaper = True
            logger.info(f"Setting wallpaper as background: {filepath}")
            
            # Show a single notification
            self.show_info_toast("Setting wallpaper...")
            
            # Start the thread to avoid blocking the UI
            threading.Thread(
                target=self._set_wallpaper_thread,
                args=(filepath,),
                daemon=True
            ).start()
            return True
            
        except Exception as e:
            logger.error(f"Error setting wallpaper as background: {e}")
            self.show_error_toast(f"Error setting wallpaper: {e}")
            self._show_wallpaper_instructions_dialog(filepath)
            self._is_setting_wallpaper = False
            return False
            
    def _detect_desktop_environment(self):
        """Detect the current desktop environment.
        
        Returns:
            str: The name of the desktop environment (gnome, kde, xfce, etc.)
        """
        desktop = os.environ.get('XDG_CURRENT_DESKTOP', '').lower()
        
        if 'gnome' in desktop or 'ubuntu' in desktop or 'pop' in desktop:
            return 'gnome'
        elif 'kde' in desktop or 'plasma' in desktop:
            return 'kde'
        elif 'xfce' in desktop:
            return 'xfce'
        elif 'cinnamon' in desktop:
            return 'cinnamon'
        elif 'mate' in desktop:
            return 'mate'
        elif 'budgie' in desktop:
            return 'budgie'
            
        # Fallback to checking for common processes
        try:
            import psutil
            for proc in psutil.process_iter(['name']):
                name = proc.info['name'].lower()
                if 'gnome' in name:
                    return 'gnome'
                elif 'kde' in name or 'plasma' in name:
                    return 'kde'
                elif 'xfce' in name:
                    return 'xfce'
        except (ImportError, psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            pass
            
        return 'unknown'
        
    def _show_wallpaper_instructions_dialog(self, filepath):
        """Show a dialog with instructions for manually setting the wallpaper"""
        try:
            # Create a dialog with instructions
            dialog = Adw.MessageDialog.new(self.props.active_window)
            dialog.set_heading("Set Wallpaper Manually")
            
            # Create message with the file path
            message = f"The wallpaper is saved at:\n{filepath}\n\nTo set it as your desktop background:\n\n"
            message += "1. Open Settings\n"
            message += "2. Go to Background\n"
            message += "3. Click Add Picture\n"
            message += "4. Navigate to the path above\n"
            message += "5. Select the wallpaper and click Open"
            
            dialog.set_body(message)
            
            # Add a Copy Path button
            dialog.add_response("copy", "Copy Path")
            dialog.add_response("close", "Close")
            dialog.set_default_response("close")
            dialog.set_close_response("close")
            
            # Handle dialog response
            def on_dialog_response(dialog, response):
                if response == "copy":
                    # Copy the path to clipboard
                    clipboard = Gdk.Display.get_default().get_clipboard()
                    clipboard.set(filepath)
                    
                    # Show confirmation toast
                    copy_toast = Adw.Toast.new("Path copied to clipboard")
                    copy_toast.set_timeout(2)
                    self.toast_overlay.add_toast(copy_toast)
                    
                    # Keep the dialog open
                    return True
                return False
            
            dialog.connect("response", on_dialog_response)
            dialog.present()
            
        except Exception as e:
            logger.error(f"Error showing instructions dialog: {e}")
            
    def show_info_toast(self, message):
        """Show an informational toast notification"""
        toast = Adw.Toast.new(message)
        toast.set_timeout(2)
        self.toast_overlay.add_toast(toast)
    
    def show_success_toast(self, message):
        """Show a success toast notification"""
        toast = Adw.Toast.new(message)
        toast.set_timeout(3)
        self.toast_overlay.add_toast(toast)
        
    def _show_wallpaper_context_menu(self, filepath, gesture, x, y):
        """Show a context menu for a wallpaper"""
        try:
            logger.info(f"Showing context menu for: {filepath}")
            
            # Create a popover menu
            popover = Gtk.PopoverMenu.new()
            
            # Create a menu model
            menu = Gio.Menu.new()
            
            # Add menu items
            menu.append("Set as Background", "app.set-background")
            menu.append("Open in Preview", "app.open-preview")
            menu.append("Copy File Path", "app.copy-path")
            
            # Set the menu model for the popover
            popover.set_menu_model(menu)
            
            # Get the widget that was clicked
            widget = gesture.get_widget()
            
            # Set up the actions
            action_group = Gio.SimpleActionGroup.new()
            
            # Set as Background action
            action_set_bg = Gio.SimpleAction.new("set-background", None)
            action_set_bg.connect("activate", lambda a, p, path=filepath: self._set_as_background(path))
            action_group.add_action(action_set_bg)
            
            # Open in Preview action
            action_preview = Gio.SimpleAction.new("open-preview", None)
            action_preview.connect("activate", lambda a, p, path=filepath: self._show_wallpaper_preview(path))
            action_group.add_action(action_preview)
            
            # Copy Path action
            action_copy = Gio.SimpleAction.new("copy-path", None)
            action_copy.connect("activate", lambda a, p, path=filepath: self._copy_file_path_to_clipboard(path))
            action_group.add_action(action_copy)
            
            # Add the action group to the widget
            widget.insert_action_group("app", action_group)
            
            # Set the parent of the popover
            popover.set_parent(widget)
            
            # Position the popover at the click coordinates
            rect = Gdk.Rectangle()
            rect.x = x
            rect.y = y
            rect.width = 1
            rect.height = 1
            popover.set_pointing_to(rect)
            
            # Show the popover
            popover.popup()
            
        except Exception as e:
            logger.error(f"Error showing context menu: {e}")
            self.show_error_toast(f"Error showing menu: {e}")
    
    def _copy_file_path_to_clipboard(self, filepath):
        """Copy a file path to the clipboard"""
        try:
            clipboard = Gdk.Display.get_default().get_clipboard()
            clipboard.set(filepath)
            
            # Show a toast notification
            self.show_success_toast("File path copied to clipboard")
            logger.info(f"Copied file path to clipboard: {filepath}")
        except Exception as e:
            logger.error(f"Error copying file path: {e}")
            self.show_error_toast(f"Error copying path: {e}")
    
    # Cache for downloaded wallpapers list and their modification times
    _wallpapers_cache = {}
    _last_cache_update = 0
    CACHE_EXPIRY = 60  # Cache expiry time in seconds
    _executor = None  # Thread pool executor for thumbnail loading

    def _get_cached_wallpapers(self):
        """Get the list of wallpapers, using cache if possible"""
        current_time = time.time()
        download_dir = self.get_wallpapers_dir()
        
        # Create directory if it doesn't exist
        if not os.path.exists(download_dir):
            os.makedirs(download_dir)
            return []
        
        # Check if cache is still valid
        if (download_dir in self._wallpapers_cache and 
            (current_time - self._last_cache_update) < self.CACHE_EXPIRY):
            return self._wallpapers_cache[download_dir]
        
        try:
            # Get list of image files with their modification times
            wallpapers = []
            for filename in os.listdir(download_dir):
                if filename.lower().endswith(('.png', '.jpg', '.jpeg', '.webp')):
                    filepath = os.path.join(download_dir, filename)
                    try:
                        mtime = os.path.getmtime(filepath)
                        wallpapers.append((filepath, mtime, filename))
                    except (OSError, Exception) as e:
                        logger.warning(f"Could not access {filepath}: {e}")
            
            # Sort by modification time (newest first)
            wallpapers.sort(key=lambda x: x[1], reverse=True)
            
            # Update cache
            self._wallpapers_cache[download_dir] = wallpapers
            self._last_cache_update = current_time
            
            return wallpapers
            
        except Exception as e:
            logger.error(f"Error getting wallpapers list: {e}")
            return []
            
    def _get_executor(self):
        """Get or create a thread pool executor for thumbnail loading"""
        if self._executor is None:
            import concurrent.futures
            # Create a thread pool with up to 4 threads
            self._executor = concurrent.futures.ThreadPoolExecutor(max_workers=4)
        return self._executor
    
    # Dictionary to track all thumbnail widgets by filepath
    _thumbnail_widgets = {}
    
    def _load_thumbnail_async(self, filepath, mtime, flowbox):
        """Load a thumbnail asynchronously using a thread pool"""
        def load_thumbnail():
            try:
                logger.debug(f"[Thread] Loading thumbnail: {filepath}")
                
                # Check if file exists and is readable
                if not os.path.exists(filepath):
                    logger.error(f"File not found: {filepath}")
                    return None
                    
                if not os.access(filepath, os.R_OK):
                    logger.error(f"No read permissions for file: {filepath}")
                    return None
                
                # Create a pixbuf for the thumbnail
                try:
                    return GdkPixbuf.Pixbuf.new_from_file_at_scale(
                        filepath,
                        200,  # width
                        200,  # height
                        True  # preserve aspect ratio
                    )
                except GLib.GError as e:
                    logger.error(f"Error creating pixbuf for {filepath}: {e}")
                    return None
                    
            except Exception as e:
                logger.error(f"Unexpected error in thumbnail loader: {e}", exc_info=True)
                return None
        
        def on_thumbnail_loaded(future):
            try:
                pixbuf = future.result()
                if pixbuf and filepath in self._thumbnail_widgets:
                    widget = self._thumbnail_widgets[filepath]
                    GLib.idle_add(self._update_thumbnail_ui, filepath, pixbuf, widget)
            except Exception as e:
                logger.error(f"Error processing thumbnail: {e}", exc_info=True)
        
        # Submit the task to the thread pool
        future = self._get_executor().submit(load_thumbnail)
        future.add_done_callback(on_thumbnail_loaded)
    
    def _update_thumbnail_ui(self, filepath, pixbuf, widget):
        """Update the UI with the loaded thumbnail"""
        try:
            logger.debug(f"Updating UI for {filepath}")
            logger.debug(f"Widget type: {type(widget).__name__}")
            logger.debug(f"Widget has overlay: {hasattr(widget, 'overlay')}")
            
            if not pixbuf or not widget or not hasattr(widget, 'overlay'):
                logger.error("Missing required parameters or widget has no overlay")
                return
            
            # Log overlay children before changes
            if hasattr(widget.overlay, 'get_first_child'):
                logger.debug("Overlay children before update:")
                child = widget.overlay.get_first_child()
                while child:
                    logger.debug(f"  - {type(child).__name__}")
                    child = child.get_next_sibling()
            
            # Create the picture widget
            logger.debug("Creating new picture widget")
            picture = Gtk.Picture.new_for_pixbuf(pixbuf)
            picture.set_size_request(200, 150)
            picture.set_can_shrink(True)
            picture.set_content_fit(Gtk.ContentFit.COVER)
            
            # Hide the spinner if it exists
            if hasattr(widget, 'spinner') and widget.spinner:
                logger.debug("Stopping spinner")
                if hasattr(widget.spinner, 'stop'):
                    widget.spinner.stop()
                spinner_box = widget.spinner.get_parent()
                if spinner_box:
                    spinner_box.set_visible(False)
            
            # Remove existing picture if any
            if hasattr(widget, 'picture') and widget.picture:
                logger.debug("Removing existing picture")
                if widget.picture.get_parent() == widget.overlay:
                    widget.overlay.remove(widget.picture)
            
            # Add the new picture
            logger.debug("Adding new picture to overlay")
            widget.picture = picture
            
            # Use appropriate method based on GTK4 version
            if hasattr(widget.overlay, 'set_child'):  # GTK 4.6+
                widget.overlay.set_child(picture)
            elif hasattr(widget.overlay, 'add_overlay'):  # Older GTK4
                widget.overlay.add_overlay(picture)
            else:  # Fallback to direct child setting
                widget.overlay.set_child(picture)
            
            picture.show()
            logger.info(f"Successfully updated thumbnail for {os.path.basename(filepath)}")
            
        except Exception as e:
            logger.error(f"Error updating UI for {filepath}: {e}", exc_info=True)
            logger.error(f"Widget dir: {dir(widget) if hasattr(widget, '__dir__') else 'No dir'}")
            if hasattr(widget, 'overlay'):
                logger.error(f"Overlay dir: {dir(widget.overlay) if hasattr(widget.overlay, '__dir__') else 'No dir'}")
                
    def _show_downloads_browser(self, force_refresh=False):
        """Show a file browser for the downloaded wallpapers
        
        Args:
            force_refresh: If True, forces a refresh of the wallpapers cache
        """
        try:
            logger.info("Showing downloads browser")
            logger.debug(f"Current working directory: {os.getcwd()}")
            logger.debug(f"Wallpapers directory: {self.get_wallpapers_dir()}")
            
            # Clear cache if force refresh is requested
            if force_refresh:
                download_dir = self.get_wallpapers_dir()
                if download_dir in self._wallpapers_cache:
                    del self._wallpapers_cache[download_dir]
            
            # Get wallpapers (will use cache if available)
            wallpapers = self._get_cached_wallpapers()
            logger.debug(f"Found {len(wallpapers)} wallpapers in cache")
            
            # Create a new window
            browser_window = Adw.Window.new()
            browser_window.set_title("Downloaded Wallpapers")
            browser_window.set_default_size(800, 600)
            
            # Create a toast overlay for notifications
            toast_overlay = Adw.ToastOverlay.new()
            
            # Create the main box
            main_box = Gtk.Box.new(Gtk.Orientation.VERTICAL, 0)
            
            # Create a header bar
            header = Adw.HeaderBar.new()
            title_widget = Adw.WindowTitle.new("Downloaded Wallpapers", f"{len(wallpapers)} wallpapers")
            header.set_title_widget(title_widget)
            
            # Add a refresh button
            refresh_button = Gtk.Button.new_from_icon_name("view-refresh-symbolic")
            refresh_button.set_tooltip_text("Refresh")
            refresh_button.connect("clicked", lambda b: self._refresh_downloads_browser(browser_window))
            header.pack_start(refresh_button)
            
            # Add an open folder button
            folder_button = Gtk.Button.new_from_icon_name("folder-open-symbolic")
            folder_button.set_tooltip_text("Open Downloads Folder")
            folder_button.connect("clicked", lambda b: self._open_downloads_folder())
            header.pack_start(folder_button)
            
            main_box.append(header)
            
            # Create a scrolled window
            scrolled = Gtk.ScrolledWindow.new()
            scrolled.set_hexpand(True)
            scrolled.set_vexpand(True)
            scrolled.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)  # Only vertical scrolling
            
            # Create a viewport to contain the flowbox
            viewport = Gtk.Viewport.new()
            viewport.set_hexpand(True)
            viewport.set_vexpand(True)
            viewport.set_scroll_to_focus(True)
            
            # Create a flowbox for the wallpapers with consistent sizing
            flowbox = Gtk.FlowBox.new()
            flowbox.set_valign(Gtk.Align.START)
            flowbox.set_selection_mode(Gtk.SelectionMode.NONE)
            flowbox.set_homogeneous(True)
            flowbox.set_column_spacing(12)
            flowbox.set_row_spacing(12)
            flowbox.set_margin_start(24)
            flowbox.set_margin_end(24)
            flowbox.set_margin_top(12)
            flowbox.set_margin_bottom(24)
            flowbox.set_halign(Gtk.Align.FILL)
            flowbox.set_hexpand(True)
            flowbox.set_max_children_per_line(4)  # Adjust based on window size
            flowbox.set_min_children_per_line(2)  # At least 2 items per row
            
            # Connect to size allocation changes to adjust the number of columns based on width
            def on_flowbox_size_changed(flowbox, pspec):
                # Get the allocation
                allocation = flowbox.get_allocation()
                if allocation.width <= 1:  # Skip invalid allocations
                    return
                    
                # Calculate number of columns based on width (200px per item + spacing)
                item_width = 200
                spacing = flowbox.get_column_spacing()
                margin = flowbox.get_margin_start() + flowbox.get_margin_end()
                available_width = allocation.width - margin
                n_columns = max(2, min(6, available_width // (item_width + spacing)))
                flowbox.set_max_children_per_line(n_columns)
                
            # Connect to the notify::allocation signal in GTK4
            flowbox.connect('notify::allocation', on_flowbox_size_changed)
            
            # Add flowbox to viewport
            viewport.set_child(flowbox)
            scrolled.set_child(viewport)
            
            # Clear any existing thumbnails
            self._thumbnail_widgets.clear()
            
            # Add wallpapers to the flowbox
            if wallpapers:
                logger.debug(f"Adding {len(wallpapers)} wallpapers to flowbox")
                for idx, (wp_path, mtime, filename) in enumerate(wallpapers, 1):
                    logger.debug(f"Processing wallpaper {idx}/{len(wallpapers)}: {filename}")
                    if not os.path.exists(wp_path):
                        logger.warning(f"Wallpaper file not found: {wp_path}")
                        continue
                        
                    # Create a box for the wallpaper item
                    box = Gtk.Box.new(Gtk.Orientation.VERTICAL, 6)
                    box.set_margin_top(6)
                    box.set_margin_bottom(6)
                    box.set_margin_start(6)
                    box.set_margin_end(6)
                    box.set_name(f"wallpaper_box_{idx}")
                    
                    # Create a frame for the image
                    frame = Gtk.Frame.new()
                    frame.set_name(f"frame_{idx}")
                    frame.set_size_request(200, 150)
                    frame.set_hexpand(True)
                    frame.set_halign(Gtk.Align.FILL)
                    frame.set_valign(Gtk.Align.FILL)
                    
                    # Create overlay for spinner and image
                    overlay = Gtk.Overlay.new()
                    overlay.set_hexpand(True)
                    overlay.set_halign(Gtk.Align.FILL)
                    overlay.set_valign(Gtk.Align.FILL)
                    
                    # Create and add spinner
                    spinner_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=0)
                    spinner_box.set_halign(Gtk.Align.CENTER)
                    spinner_box.set_valign(Gtk.Align.CENTER)
                    spinner_box.set_hexpand(True)
                    spinner_box.set_vexpand(True)
                    
                    spinner = Gtk.Spinner.new()
                    spinner.set_size_request(32, 32)
                    spinner.start()
                    spinner_box.append(spinner)
                    
                    # Add spinner to overlay
                    overlay.set_child(spinner_box)
                    
                    # Set overlay as frame's child
                    frame.set_child(overlay)
                    
                    # Store references
                    frame.overlay = overlay
                    frame.spinner = spinner
                    frame.filepath = wp_path
                    frame.picture = None
                    
                    # Store in our widgets dictionary
                    self._thumbnail_widgets[wp_path] = frame
                    
                    # Add the frame to the box
                    box.append(frame)
                    
                    # Add filename label
                    label = Gtk.Label.new(filename)
                    label.set_ellipsize(3)
                    label.set_max_width_chars(20)
                    label.set_halign(Gtk.Align.START)
                    label.set_margin_start(4)
                    label.set_margin_end(4)
                    label.set_margin_bottom(4)
                    label.set_hexpand(True)
                    box.append(label)
                    
                    # Set up click handlers
                    frame.set_cursor(Gdk.Cursor.new_from_name("pointer"))
                    
                    click = Gtk.GestureClick.new()
                    click.connect("pressed", 
                    lambda g, n, x, y, path=wp_path, widget=frame: self._show_wallpaper_preview(path, widget.get_root()))
                    frame.add_controller(click)
                    
                    context_menu = Gtk.GestureClick.new()
                    context_menu.set_button(3)
                    context_menu.connect("pressed", 
                        lambda g, n, x, y, path=wp_path: self._show_wallpaper_context_menu(path, g, x, y))
                    frame.add_controller(context_menu)
                    
                    # Add to flowbox
                    flowbox.append(box)
                    
                    # Start loading the thumbnail
                    self._load_thumbnail_async(wp_path, mtime, flowbox)
            else:
                # Show a message if no wallpapers are found
                empty_box = Gtk.Box.new(Gtk.Orientation.VERTICAL, 10)
                empty_box.set_valign(Gtk.Align.CENTER)
                empty_box.set_halign(Gtk.Align.CENTER)
                
                # Add an icon
                icon = Gtk.Image.new_from_icon_name("folder-pictures-symbolic")
                icon.set_pixel_size(64)
                empty_box.append(icon)
                
                # Add a label
                label = Gtk.Label.new("No downloaded wallpapers found")
                label.set_margin_top(10)
                empty_box.append(label)
                
                # Add a subtitle
                subtitle = Gtk.Label.new(f"Download wallpapers to {self.get_wallpapers_dir()}")
                empty_box.append(subtitle)
                
                flowbox.append(empty_box)
            
            # Add the flowbox to the scrolled window
            scrolled.set_child(flowbox)
            
            # Add the scrolled window to the main box
            main_box.append(scrolled)
            
            # Set the toast overlay's child
            toast_overlay.set_child(main_box)
            
            # Set the window's content
            browser_window.set_content(toast_overlay)
            
            # Show the window
            browser_window.present()
            
        except Exception as e:
            logger.error(f"Error showing downloads browser: {e}")
            logger.error(f"Error type: {type(e).__name__}")
            import traceback
            logger.error(f"Traceback: {traceback.format_exc()}")
            self.show_error_toast(f"Error showing downloads: {e}")
    
    def _refresh_downloads_browser(self, window):
        """Refresh the downloads browser window"""
        try:
            # Clear the cache to force a refresh
            download_dir = self.get_wallpapers_dir()
            if download_dir in self._wallpapers_cache:
                del self._wallpapers_cache[download_dir]
            
            # Close the current window and open a new one
            window.destroy()
            self._show_downloads_browser()
        except Exception as e:
            logger.error(f"Error refreshing downloads browser: {e}")
            self.show_error_toast(f"Error refreshing: {e}")
    
    def _copy_downloads_path_to_clipboard(self, window):
        """Copy the downloads path to clipboard"""
        try:
            download_dir = self.get_wallpapers_dir()
            clipboard = Gdk.Display.get_default().get_clipboard()
            clipboard.set(download_dir)
            
            # Get the toast overlay from the browser_data
            if hasattr(window, 'browser_data') and hasattr(window.browser_data, 'toast_overlay'):
                toast_overlay = window.browser_data.toast_overlay
                
                # Show a toast notification
                toast = Adw.Toast.new("Folder path copied to clipboard")
                toast.set_timeout(2)
                toast_overlay.add_toast(toast)
            else:
                # Fallback to the main application toast overlay
                toast = Adw.Toast.new("Folder path copied to clipboard")
                toast.set_timeout(2)
                self.toast_overlay.add_toast(toast)
                
            logger.info(f"Copied path to clipboard: {download_dir}")
        except Exception as e:
            logger.error(f"Error copying path to clipboard: {e}")
            self.show_error_toast(f"Error copying path: {e}")
    
    def _open_downloads_folder(self):
        """Open the downloads folder in the system file manager."""
        try:
            downloads_dir = self.get_wallpapers_dir()
            logger.info(f"Opening downloads folder: {downloads_dir}")
            
            # Try to open with xdg-open first (works on most Linux systems)
            try:
                subprocess.Popen(['xdg-open', downloads_dir], 
                              stdout=subprocess.PIPE, 
                              stderr=subprocess.PIPE)
                return True
            except Exception as e:
                logger.warning(f"Failed to open with xdg-open: {e}")
            
            # Fall back to using Gtk.show_uri
            try:
                file = Gio.File.new_for_path(downloads_dir)
                Gtk.show_uri(None, file.get_uri(), Gdk.CURRENT_TIME)
                return True
            except Exception as e:
                logger.error(f"Failed to open with Gtk.show_uri: {e}")
                self.show_error_toast(f"Could not open downloads folder: {e}")
                return False
                
        except Exception as e:
            logger.error(f"Error opening downloads folder: {e}")
            self.show_error_toast(f"Error: {str(e)}")
            return False

    def _on_browser_window_resize(self, window, param):
        """Handle window resize events for the downloads browser"""
        try:
            # Access the flowbox through the browser_data attribute
            if hasattr(window, 'browser_data') and hasattr(window.browser_data, 'flowbox'):
                # Let the flowbox handle its own layout based on available space
                # This matches the behavior of the main window's flowbox
                # No need to manually calculate columns
                pass
            else:
                logger.warning("Could not find flowbox in browser_data")
        except Exception as e:
            logger.error(f"Error handling window resize: {e}")
        
    def _try_open_with_app_info(self, folder_path, command):
        """Try to open a folder using Gio.AppInfo.create_from_commandline"""
        try:
            logger.info(f"Trying to open folder with command: {command}")
            app_info = Gio.AppInfo.create_from_commandline(
                f"{command} {folder_path}",
                f"Open {folder_path}",
                Gio.AppInfoCreateFlags.SUPPORTS_URIS
            )
            
            if app_info:
                logger.info(f"Created AppInfo with command: {command}")
                # Create a launch context with proper environment
                context = Gio.AppLaunchContext.new()
                success = app_info.launch([], context)
                logger.info(f"Launch result: {'Success' if success else 'Failed'}")
                return success
            return False
        except Exception as e:
            logger.error(f"Error opening folder with {command}: {e}")
            return False
            
    def _try_open_with_subprocess(self, folder_path):
        """Try to open a folder using the Flatpak portal system"""
        try:
            logger.info("Trying to open folder with Flatpak portal")
            
            # Create a file URI for the folder
            from urllib.parse import quote
            encoded_path = quote(folder_path)
            file_uri = f"file://{encoded_path}"
            logger.info(f"Opening URI: {file_uri}")
            
            # Use Gtk.UriLauncher which is designed to work with Flatpak portals
            launcher = Gtk.UriLauncher.new(file_uri)
            
            # Launch asynchronously to avoid blocking the UI
            def launch_callback(source_object, result, user_data):
                try:
                    success = launcher.launch_finish(result)
                    logger.info(f"Portal launch result: {'Success' if success else 'Failed'}")
                except Exception as e:
                    logger.error(f"Error in portal launch: {e}")
            
            # Launch with the active window as parent
            launcher.launch(self.props.active_window, None, launch_callback, None)
            
            # Return True to indicate we've started the process
            # The actual success/failure will be logged in the callback
            return True
            
        except Exception as e:
            logger.error(f"Error setting up portal launch: {e}")
            return False
        
    def show_error_toast(self, message):
        toast = Adw.Toast.new(message)
        toast.set_timeout(5)
        self.toast_overlay.add_toast(toast)
        return False
    
    def on_view_downloads(self, action, param):
        logger.info("View Downloads clicked")
        
        # Show the downloads browser with a forced refresh to show latest downloads
        self._show_downloads_browser(force_refresh=True)

    def on_about(self, action, param):
        logger.info("About clicked")
        
        # Create a custom dialog
        about = Gtk.Dialog(
            title="About WSelector",
            transient_for=self.props.active_window,
            modal=True,
            default_width=400,
            default_height=-1
        )
        
        # Create the main content area with proper spacing
        content = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=18, 
                         margin_top=24, margin_bottom=18, 
                         margin_start=24, margin_end=24)
        
        # Header section
        header_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=16)
        
        # App icon (using the app's icon)
        icon = Gtk.Image.new_from_icon_name("io.github.Cookiiieee.WSelector")
        icon.set_pixel_size(64)
        icon.get_style_context().add_class("about-icon")
        header_box.append(icon)
        
        # App info section
        info_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=4)
        
        # App title
        name_label = Gtk.Label(label="<b>WSelector</b>", 
        use_markup=True, halign=Gtk.Align.START)
        name_label.get_style_context().add_class("title-1")
        info_box.append(name_label)
        
        # App version
        version_label = Gtk.Label(label="Version 0.1.3", 
        halign=Gtk.Align.START, 
        opacity=0.8)
        info_box.append(version_label)
        
        # Developer info
        dev_label = Gtk.Label(label="Developed by Phillip Cook • 2025",
        halign=Gtk.Align.START,
        margin_top=8,
        opacity=0.7)
        info_box.append(dev_label)
        
        header_box.append(info_box)
        content.append(header_box)
        
        # Description
        desc_label = Gtk.Label(
            label="Browse, Download and Manage your Wallpapers with ease.",
            wrap=True,
            margin_top=12,
            justify=Gtk.Justification.CENTER
        )
        desc_label.set_max_width_chars(50)
        content.append(desc_label)
        
        # Action buttons
        actions_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, 
        spacing=8,
        margin_top=12,
        halign=Gtk.Align.CENTER)
        
        def open_uri(button, uri):
            try:
                # First try with xdg-open which is more reliable in Flatpak
                import subprocess
                subprocess.Popen(['xdg-open', uri])
            except Exception as e:
                # Fallback to Gtk.show_uri if xdg-open fails
                try:
                    Gtk.show_uri(
                        self.props.active_window,
                        uri,
                        Gdk.CURRENT_TIME
                    )
                except Exception as e:
                    logger.error(f"Failed to open URI: {e}")
        
        # Website button
        website_btn = Gtk.Button(label="🌐 Project Website")
        website_btn.connect("clicked", open_uri, "https://github.com/Cookiiieee/WSelector/")
        website_btn.set_halign(Gtk.Align.CENTER)
        website_btn.set_hexpand(True)
        actions_box.append(website_btn)
        
        # Report Issue button
        report_btn = Gtk.Button(label="🐞 Report an Issue")
        report_btn.connect("clicked", open_uri, "https://github.com/Cookiiieee/WSelector/issues/new")
        report_btn.set_halign(Gtk.Align.CENTER)
        report_btn.set_hexpand(True)
        actions_box.append(report_btn)
        
        # Support button
        support_btn = Gtk.Button(label="☕ Support the Project")
        support_btn.connect("clicked", open_uri, "https://buymeacoffee.com/cookiiieee")
        support_btn.get_style_context().add_class("suggested-action")
        support_btn.set_halign(Gtk.Align.CENTER)
        support_btn.set_hexpand(True)
        actions_box.append(support_btn)
        
        content.append(actions_box)
        
        # Credits
        credits_label = Gtk.Label(
            label="<small>With gratitude to the Wallhaven.cc team for their amazing wallpaper collection.</small>",
            use_markup=True,
            margin_top=18,
            opacity=0.6,
            justify=Gtk.Justification.CENTER
        )
        content.append(credits_label)
        
        # Add close button to action area
        close_btn = about.add_button("Close", Gtk.ResponseType.CLOSE)
        close_btn.connect("clicked", lambda *_: about.destroy())
        
        # Set the content and show the dialog
        about.set_child(content)
        about.present()


    def show_error(self, message):
        logger.error(message)

    def on_window_size_changed(self, widget, param):
        pass

    def do_startup(self):
        Adw.StyleManager.get_default().set_color_scheme(Adw.ColorScheme.PREFER_LIGHT)
        Gtk.Application.do_startup(self)


if __name__ == "__main__":
    app = WSelectorApp("io.github.Cookiiieee.WSelector", Gio.ApplicationFlags.FLAGS_NONE)
    try:
        app.run(sys.argv)
    except Exception as e:
        logger.error(f"An error occurred: {e}")
        sys.exit(1)