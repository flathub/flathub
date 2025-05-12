#!/usr/bin/env python3

import gi, sys
import urllib.request
import os

gi.require_version(namespace='Gtk', version='4.0')
gi.require_version(namespace='Adw', version='1')

from gi.repository import Adw, Gio, Gtk, GLib


Adw.init()


def download_image(url, save_as):
    urllib.request.urlretrieve(url, save_as)


class ImageTab:
    """Class to handle image tabs with zoom functionality"""
    
    def __init__(self, filepath):
        # Create container box
        self.box = Gtk.Box.new(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        self.box.set_hexpand(True)
        self.box.set_vexpand(True)

        # Create picture widget
        self.picture = Gtk.Picture.new_for_file(Gio.File.new_for_path(filepath))
        self.picture.set_can_shrink(True)
        self.picture.set_content_fit(Gtk.ContentFit.CONTAIN)
        self.picture.set_hexpand(True)
        self.picture.set_vexpand(True)

        # Store initial size for zooming
        texture = self.picture.get_paintable()
        if texture:
            self.picture.original_width = texture.get_intrinsic_width()
            self.picture.original_height = texture.get_intrinsic_height()
            self.picture.scale = 1.0

        # Create scrolled window
        self.scroll = Gtk.ScrolledWindow()
        self.scroll.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        self.scroll.set_hexpand(True)
        self.scroll.set_vexpand(True)
        self.scroll.set_child(self.box)
        self.scroll.set_kinetic_scrolling(True)
        self.scroll.set_overlay_scrolling(True)

        # Add picture to box
        self.box.append(self.picture)

    def zoom_in(self, button):
        """Zoom in the picture and maintain view center"""
        texture = self.picture.get_paintable()
        if texture and hasattr(self.picture, 'scale'):
            self._zoom(self.picture.scale * 1.2)

    def zoom_out(self, button):
        """Zoom out the picture and maintain view center"""
        texture = self.picture.get_paintable()
        if texture and hasattr(self.picture, 'scale'):
            self._zoom(self.picture.scale / 1.2)

    def zoom_fit(self, button):
        """Reset zoom to fit window"""
        texture = self.picture.get_paintable()
        if texture:
            self.picture.scale = 1.0
            self.picture.set_size_request(-1, -1)

    def _zoom(self, new_scale):
        """Internal method to handle zooming"""
        # Get the scrolled window adjustments
        hadj = self.scroll.get_hadjustment()
        vadj = self.scroll.get_vadjustment()
        
        # Calculate view center as percentage
        rel_x = (hadj.get_value() + hadj.get_page_size() / 2) / max(1, hadj.get_upper())
        rel_y = (vadj.get_value() + vadj.get_page_size() / 2) / max(1, vadj.get_upper())
        
        # Apply zoom with limits
        self.picture.scale = max(min(new_scale, 5.0), 0.1)
        new_width = int(self.picture.original_width * self.picture.scale)
        new_height = int(self.picture.original_height * self.picture.scale)
        self.picture.set_size_request(new_width, new_height)
        
        # Update scroll position after resize
        def update_scroll():
            new_upper_x = hadj.get_upper()
            new_upper_y = vadj.get_upper()
            
            hadj.set_value(max(0, min(
                new_upper_x * rel_x - hadj.get_page_size() / 2,
                new_upper_x - hadj.get_page_size()
            )))
            vadj.set_value(max(0, min(
                new_upper_y * rel_y - vadj.get_page_size() / 2,
                new_upper_y - vadj.get_page_size()
            )))
            return False
        
        GLib.timeout_add(50, update_scroll)


class AppWindow(Gtk.ApplicationWindow):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

        self.set_title(title='Eve Daily Sov Maps')
        self.set_default_size(width=int(928), height=int(1024))

        # Create a TabView
        self.tabView = Adw.TabView.new()
        
        # Create main content box that will contain the TabView
        content_box = Gtk.Box.new(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        
        # Add TabView to the content box
        content_box.append(self.tabView)

        # Create TabBar and add it to content box
        tab_bar = Adw.TabBar.new()
        tab_bar.set_view(self.tabView)
        #content_box.prepend(tab_bar)

        # Create TabOverview as the window's main child
        self.tabOverview = Adw.TabOverview.new()
        self.tabOverview.set_view(self.tabView)
        self.tabOverview.set_child(content_box)  # Set content_box as child of TabOverview
        self.set_child(self.tabOverview)  # Set TabOverview as window's child

        # Header bar setup
        header_bar = Adw.HeaderBar.new()
        self.set_titlebar(titlebar=header_bar)

        # Add toggle button to header
        toggle_button = Gtk.Button.new_with_label("Toggle Overview")
        toggle_button.connect("clicked", self.on_toggle_tab_overview)
        header_bar.pack_start(toggle_button)

        # Add menu button to header
        menu_button_model = Gio.Menu()
        menu_button_model.append('About', 'app.about')
        menu_button = Gtk.MenuButton.new()
        menu_button.set_icon_name(icon_name='open-menu-symbolic')
        menu_button.set_menu_model(menu_model=menu_button_model)
        header_bar.pack_end(menu_button)

        # Add zoom controls to header bar
        zoom_out_button = Gtk.Button.new_from_icon_name("zoom-out-symbolic")
        zoom_in_button = Gtk.Button.new_from_icon_name("zoom-in-symbolic")
        zoom_fit_button = Gtk.Button.new_from_icon_name("zoom-fit-best-symbolic")

        header_bar.pack_start(zoom_out_button)
        header_bar.pack_start(zoom_fit_button)
        header_bar.pack_start(zoom_in_button)

        # Create and add image tab
        #influence_tab = ImageTab("./influence.png")
        cache_dir = os.path.join(GLib.get_user_cache_dir(), 'cloud.pulfer.EveInfluenceMap')
        influence_tab = ImageTab(os.path.join(cache_dir, "influence.png"))
        tab_page = self.tabView.append(influence_tab.scroll)
        tab_page.set_title("Influence Map")

        # Connect zoom buttons to current tab
        zoom_out_button.connect("clicked", influence_tab.zoom_out)
        zoom_in_button.connect("clicked", influence_tab.zoom_in)
        zoom_fit_button.connect("clicked", influence_tab.zoom_fit)

        # Store reference to image tab
        self.current_image_tab = influence_tab

        # Create the tabs
        #coalition = ImageTab("./coalitioninfluence.png")
        coalition = ImageTab(os.path.join(cache_dir, "coalitioninfluence.png"))
        tab_page = self.tabView.append(coalition.scroll)
        tab_page.set_title("Coalition Influence")
        zoom_out_button.connect("clicked", coalition.zoom_out)
        zoom_in_button.connect("clicked", coalition.zoom_in)
        zoom_fit_button.connect("clicked", coalition.zoom_fit)

    def on_toggle_tab_overview(self, button):
        """Toggle the TabOverview open/closed state."""
        is_open = self.tabOverview.get_open()
        self.tabOverview.set_open(not is_open)

    def on_create_tab(self, tab_overview):
        """Handle new tab creation."""
        new_label = Gtk.Label.new("Character Name")
        new_tab_page = self.tabView.append(new_label)
        new_tab_page.set_title("New Tab")
        return new_tab_page


class App(Adw.Application):

    def __init__(self):
        super().__init__(application_id='cloud.pulfer', flags=Gio.ApplicationFlags.FLAGS_NONE)
        self.create_action('quit', self.exit_app, ['<primary>q'])
        self.create_action('about', self.on_about_action)

    def on_about_action(self, action, param):
        dialog = Adw.AboutDialog.new()
        dialog.set_application_name('Eve Daily Sov Maps')
        dialog.set_version('1.0.0')
        dialog.set_developer_name('Patrick Pulfer')
        dialog.set_license_type(Gtk.License(Gtk.License.MIT_X11))
        dialog.set_comments('Eve Online daily sov map viewer.')
        dialog.set_website('https://evewho.com/character/185294134')
        dialog.set_copyright('© 2025 Patrick Pulfer')
        dialog.set_developers(['Patrick Pulfer https://evewho.com/character/185294134'])
        dialog.set_application_icon('help-about-symbolic')
        dialog.present()

    
    def do_activate(self):
        win = self.props.active_window
        if not win:
            win = AppWindow(application=self)
        win.present()

    def do_startup(self):
        Gtk.Application.do_startup(self)

    def do_shutdown(self):
        Gtk.Application.do_shutdown(self)

    def exit_app(self, action, param):
        self.quit()

    def create_action(self, name, callback, shortcuts=None):
        action = Gio.SimpleAction.new(name, None)
        action.connect('activate', callback)
        self.add_action(action)
        if shortcuts:
            self.set_accels_for_action(f'app.{name}', shortcuts)


def main(version):
    cache_dir = GLib.get_user_cache_dir()
    app_cache_dir = os.path.join(cache_dir, 'cloud.pulfer.EveInfluenceMap')
    os.makedirs(app_cache_dir, exist_ok=True)

    influence_map = os.path.join(app_cache_dir, 'influence.png')
    coalition_map = os.path.join(app_cache_dir, 'coalitioninfluence.png')

    download_image('https://www.verite.space/maps/influence/influence.png', influence_map)
    download_image('https://www.verite.space/maps/coalition/coalitioninfluence.png', coalition_map)
    app = App()
    #app.run(sys.argv)
    return app.run(sys.argv)


if __name__ == '__main__':
    main(1)