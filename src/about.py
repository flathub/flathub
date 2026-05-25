import gi

gi.require_version("Gtk", "4.0")
gi.require_version("Adw", "1")

from gi.repository import Gtk, Adw


@Gtk.Template(resource_path="/jp/co/masatn/ImageViewer/about.ui")
class ImageViewerAboutDialog(Adw.AboutDialog):
    __gtype_name__ = "ImageViewerAboutDialog"

    def __init__(self):
        super().__init__()
