import gi
gi.require_version("Gtk", "4.0")
gi.require_version("Adw", "1")
gi.require_version("WebKit", "6.0")
gi.require_version("GdkPixbuf", "2.0")
from gi.repository import Gtk, Adw, WebKit, Gio, GdkPixbuf

BASE64_ICON = ""

BUS_NAME = "com.mudeprolinux.whakarere"

class WebViewWindow(Gtk.Window):
    def __init__(self, app):
        super().__init__(application=app)
        self.set_default_size(800, 600)

        header_bar = Adw.HeaderBar()
        
        # Create a vertical box to hold title and subtitle
        title_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=2)

        title_label = Gtk.Label(label="Whakarere")
        title_label.add_css_class("title")

        subtitle_label = Gtk.Label(label="GTK4 WhatsApp Client")
        subtitle_label.add_css_class("subtitle")

        title_box.append(title_label)
        title_box.append(subtitle_label)

        header_bar.set_title_widget(title_box)

        self.set_titlebar(header_bar)

        # Initialize WebView and load a URL
        self.webview = WebKit.WebView()
        self.set_child(self.webview)
        self.webview.load_uri("https://web.whatsapp.com")

class WebApp(Gtk.Application):
    def __init__(self):
        super().__init__(application_id=BUS_NAME)
        
    def do_activate(self):
        win = self.props.active_window
        if not win:
            win = WebViewWindow(self)
        win.present()

    def do_startup(self):
        Gtk.Application.do_startup(self)
        Adw.init()  # Initialize libadwaita

if __name__ == "__main__":
    app = WebApp()
    exit_status = app.run()
