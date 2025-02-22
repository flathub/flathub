import gi
import threading
import webbrowser
import requests
import json
import re
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, Gdk, GLib


        
class SearchBar(Gtk.Window):
    def __init__(self):
        super().__init__(title="Search")
        self.set_default_size(700, 20)
        #self.set_border_width(10)

        self.apply_css()

        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.add(vbox)

        # Styled Search Entry
        self.entry = Gtk.Entry()
        self.set_decorated(False)  # Disable borders and title bar
        self.entry.set_placeholder_text("Type to search...")
        self.entry.set_name("search_entry")  # Name for CSS
        self.entry.connect("changed", self.start_suggestion_thread)
        self.entry.connect("activate", self.search_google)
        vbox.pack_start(self.entry, False, False, 0)
        
        self.set_position(Gtk.WindowPosition.CENTER)

        # Styled ListBox for Suggestions
        self.listbox = Gtk.ListBox()
        self.listbox.set_name("suggestion_list")
        self.listbox.connect("row-activated", self.select_suggestion)
        vbox.pack_start(self.listbox, True, True, 0)

        self.suggestions = []
        self.is_loading = False
        
        # Close Window on 'Esc' Press
        self.connect("key-press-event", self.on_key_press)
        
        self.selected_index = -1  # Track the currently selected suggestion
        
        self.navigating = False  # Flag to track if navigating through suggestions
        
        self.entry.connect("focus-out-event", self.on_focus_out)

    def on_focus_out(self, widget, event):
        self.close()


        
    def on_key_press(self, widget, event):
        """Handles global key events (Esc, arrow keys, Tab for navigation)"""

        # Close on Escape
        if event.keyval == Gdk.KEY_Escape:
            self.close()
            return True

        # If no suggestions, do nothing
        if not self.suggestions:
            return False

        # Handle arrow key navigation
        if event.keyval in [Gdk.KEY_Up, Gdk.KEY_Down]:
            if event.keyval == Gdk.KEY_Up and self.selected_index > 0:
                self.selected_index -= 1
            elif event.keyval == Gdk.KEY_Down and self.selected_index < len(self.suggestions) - 1:
                self.selected_index += 1

            self.navigating = True  # Prevent fetching new suggestions
            self.update_entry_from_suggestion(append=True)

            return True  # Stop further event propagation

        # Handle Tab Key Navigation
        if event.keyval == Gdk.KEY_Tab:
            shift_pressed = event.state & Gdk.ModifierType.SHIFT_MASK  # Detect Shift key

            if shift_pressed:  # Shift + Tab (Navigate Up)
                if self.selected_index > 0:
                    self.selected_index -= 1
            else:  # Tab (Navigate Down)
                if self.selected_index < len(self.suggestions) - 1:
                    self.selected_index += 1

            self.navigating = True  # Prevent fetching new suggestions
            self.update_entry_from_suggestion(append=False)
            return True  # Stop further event propagation

        return False  # Allow other key events to process

        
        
    def update_entry_from_suggestion(self, append=False):
        """Update entry with the currently selected suggestion.
        If append=True, preserve additional typed text.
        """
        if 0 <= self.selected_index < len(self.suggestions):
            suggestion = self.suggestions[self.selected_index]
            current_text = self.entry.get_text()

            if append:  
                # Preserve user's typed text, append it after the selected suggestion
                suffix = current_text[len(suggestion):] if current_text.startswith(suggestion) else ""
                self.entry.set_text(suggestion + suffix)
            else:
                self.entry.set_text(suggestion)

        self.entry.set_position(-1)  # Move cursor to end


        
        
    def apply_css(self):
        """Apply modern GTK styling"""
        css_provider = Gtk.CssProvider()
        css_provider.load_from_data(
            b"""
            #search_entry {
                font-size: 18px;
                padding: 10px;
                background-color: #2E2E2E;
                color: white;
                border: 2px solid #4A90E2;
            }
            #search_entry:focus {
                border: 2px solid #76A9FA;
                background-color: #383838;
            }
            #suggestion_list {
                background-color: #2E2E2E;
            }
            row {
                padding: 8px;
                font-size: 16px;
                color: white;
                background-color: transparent;
                border-radius: 6px;
            }
            row:hover, row:selected {
                background-color: #4A90E2;
            }
            """
        )

        screen = Gdk.Screen.get_default()
        context = Gtk.StyleContext()
        context.add_provider_for_screen(screen, css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

    def start_suggestion_thread(self, widget):
        """Runs get_suggestions in a separate thread, but only if not navigating via arrow keys."""
        if self.navigating:
            self.navigating = False  # Reset flag when user types again
            return  # Do not fetch new suggestions

        query = self.entry.get_text().strip()
        if query:
            threading.Thread(target=self.get_suggestions, args=(query,), daemon=True).start()
        else:
            self.clear_suggestions()

    def get_suggestions(self, query):
        """Fetch Google autocomplete suggestions asynchronously"""
        url = f"https://suggestqueries.google.com/complete/search?client=firefox&q={query}"
        try:
            response = requests.get(url, timeout=1.5)
            suggestions = json.loads(response.text)[1]
        except:
            suggestions = []

        GLib.idle_add(self.update_suggestions, suggestions)

    def update_suggestions(self, suggestions):
        """Update the UI dynamically"""
        self.clear_suggestions()
        self.suggestions = suggestions
        self.suggestions = [s for s in suggestions if s != self.entry.get_text().strip()]  # Remove duplicate suggestion
        self.selected_index = -1  # Reset index to start from the top

        for suggestion in suggestions:
            row = Gtk.ListBoxRow()
            label = Gtk.Label(label=suggestion)
            label.set_xalign(0)
            row.add(label)
            row.show_all()
            self.listbox.add(row)
            

            


    def clear_suggestions(self):
        """Remove previous suggestions"""
        self.listbox.foreach(lambda row: self.listbox.remove(row))

    def select_suggestion(self, listbox, row):
        """Handle clicking a suggestion"""
        text = row.get_child().get_text()
        self.entry.set_text(text)
        self.search_google(None)


    def search_google(self, widget):
        """Open the query in a browser, either as a direct URL or a custom site search"""
        query = self.entry.get_text().strip()

        if not query:
            return

        # Custom Site Searches (e.g., youtube/abc searches "abc" on YouTube)	
        if "/" in query:
            site, search_term = query.split("/", 1) 
        
            site_search_map = {
                "youtube": f"https://www.youtube.com/results?search_query={search_term}",
                "youtube.com": f"https://www.youtube.com/results?search_query={search_term}",
                "google": f"https://www.google.com/search?q={search_term}",
                "github": f"https://github.com/search?q={search_term}",
                "reddit": f"https://www.reddit.com/search/?q={search_term}",
                "amazon": f"https://www.amazon.com/s?k={search_term}",
                "flipkart": f"https://www.flipkart.com/search?q={search_term}",
                "stackoverflow": f"https://stackoverflow.com/search?q={search_term}",
                "wikipedia": f"https://en.wikipedia.org/wiki/Special:Search?search={search_term}",

            }

            if site in site_search_map:
                url = site_search_map[site]
            else:
                url = f"https://www.google.com/search?q={query}"  

        # Direct URL Opening
        elif re.match(r"^(https?:\/\/)?([a-zA-Z0-9.-]+\.[a-zA-Z]{2,})$", query):
            url = query if query.startswith(("http://", "https://")) else f"http://{query}"

        # Default Google Search
        else:
            url = f"https://www.google.com/search?q={query}"

        webbrowser.open_new_tab(url)
        self.close()


win = SearchBar()
win.connect("destroy", Gtk.main_quit)
win.show_all()
Gtk.main()
