import gi
gi.require_version('Gtk', '4.0')
from gi.repository import Gtk, GdkPixbuf, Gdk

class GalleryWindow(Gtk.ApplicationWindow):
    def __init__(self, app):
        super().__init__(title="Gallery", application=app)
        self.set_default_size(800, 600)

        # Create a grid to organize widgets
        grid = Gtk.Grid()
        self.set_child(grid)

        # Create a header bar
        header_bar = Gtk.HeaderBar()
        header_bar.set_show_close_button(True)
        header_bar.props.title = "Gallery"
        self.set_titlebar(header_bar)

        # Create a file chooser button to select an image
        file_button = Gtk.FileChooserButton()
        file_button.set_title("Select an Image")
        file_button.set_filter(self.create_filter())
        file_button.connect("file-set", self.on_file_set)
        header_bar.pack_start(file_button)

        # Create a search entry
        search_entry = Gtk.SearchEntry()
        search_entry.connect("search-changed", self.on_search_changed)
        header_bar.pack_end(search_entry)

        # Create a scrolled window to hold the image widget
        scrolled_window = Gtk.ScrolledWindow()
        scrolled_window.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        grid.attach(scrolled_window, 0, 1, 1, 1)

        # Create an image widget to display the selected image
        self.image = Gtk.Image()
        self.image.connect("button-press-event", self.on_image_button_press)
        scrolled_window.set_child(self.image)

    def create_filter(self):
        # Create a filter for the file chooser to only show image files
        filter = Gtk.FileFilter()
        filter.set_name("Image files")
        filter.add_mime_type("image/png")
        filter.add_mime_type("image/jpeg")
        filter.add_pattern("*.png")
        filter.add_pattern("*.jpg")
        return filter

    def on_file_set(self, button):
        # Set the image widget to display the selected image
        file = button.get_file()
        self.image.set_from_file(file.get_path())

    def on_search_changed(self, entry):
        # Filter the displayed image based on the search text
        text = entry.get_text().lower()
        if text:
            pixbuf = self.image.get_pixbuf()
            if pixbuf:
                # Convert the image to grayscale
                pixbuf = pixbuf.copy()
                pixbuf = pixbuf.convert(GdkPixbuf.Colorspace.GRAY, False, 0, 0, 0)
                # Search for the text in the grayscale image
                width = pixbuf.get_width()
                height = pixbuf.get_height()
                pixels = pixbuf.get_pixels()
                for y in range(height):
                    for x in range(width):
                        i = y * width + x
                        pixel = pixels[i]
                        if pixel < 128:
                            pixels[i] = 0
                        else:
                            pixels[i] = 255
                pixbuf = GdkPixbuf.Pixbuf.new_from_data(pixels, GdkPixbuf.Colorspace.GRAY, False, 8, width, height, width)
                # Set the image widget to display the filtered image
                self.image.set_from_pixbuf(pixbuf)
        else:
            # Reset the image widget to display the original image
            file = self.get_child().get_child().get_file()
            self.image.set_from_file(file.get_path())

    def on_image_button_press(self, widget, event):
        if event.button == Gdk.BUTTON_SECONDARY:
            # Show a context menu with copy text option
            menu = Gtk.Menu()
            copy_text_item = Gtk.MenuItem(label="Copy Text")
            copy_text_item.connect("activate", self.on_copy_text_activate)
            menu.append(copy_text_item)
            menu.show_all()
            menu.popup_at_pointer()

    def on_copy_text_activate(self, menu_item):
        # Copy text from the image to clipboard
        clipboard = Gtk.Clipboard.get_default(Gdk.Display.get_default())
        pixbuf = self.image.get_pixbuf()
        if pixbuf:
            # Convert the image to grayscale
            pixbuf = pixbuf.copy()
            pixbuf = pixbuf.convert(GdkPixbuf.Colorspace.GRAY, False, 0, 0, 0)
            # Recognize text from the grayscale image
            width = pixbuf.get_width()
            height = pixbuf.get_height()
            pixels = pixbuf.get_pixels()
            text = ""
            for y in range(height):
                for x in range(width):
                    i = y * width + x
                    pixel = pixels[i]
                    if pixel < 128:
                        text
