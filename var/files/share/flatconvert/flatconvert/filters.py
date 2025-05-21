from gi.repository import Gtk


def filters(dialog):
    # Filtre pour les images PNG, JPEG et vidéos MP4
    filter_media = Gtk.FileFilter()
    filter_media.set_name("Formats pris en charge")
    filter_media.add_mime_type("image/png")
    filter_media.add_mime_type("image/jpeg")
    filter_media.add_mime_type("video/mp4")
    filter_media.add_mime_type("video/webm")
    dialog.add_filter(filter_media)

    # Filtre pour les images PNG, JPEG et vidéos MP4
    filter_jpeg = Gtk.FileFilter()
    filter_jpeg.set_name("Images JPEG")
    filter_jpeg.add_mime_type("image/jpeg")
    dialog.add_filter(filter_jpeg)

    # Filtre pour les images PNG, JPEG et vidéos MP4
    filter_png = Gtk.FileFilter()
    filter_png.set_name("Images PNG")
    filter_png.add_mime_type("image/png")
    dialog.add_filter(filter_png)

    # Filtre pour les images PNG, JPEG et vidéos MP4
    filter_mp4 = Gtk.FileFilter()
    filter_mp4.set_name("Vidéos MP4")
    filter_mp4.add_mime_type("video/mp4")
    dialog.add_filter(filter_mp4)

    # Filtre pour les images PNG, JPEG et vidéos MP4
    filter_webm = Gtk.FileFilter()
    filter_webm.set_name("Vidéos WEBM")
    filter_webm.add_mime_type("video/webm")
    dialog.add_filter(filter_webm)

    svg_filter = Gtk.FileFilter()
    svg_filter.set_name("Fichiers SVG")
    svg_filter.add_mime_type("image/svg+xml")
    dialog.add_filter(svg_filter)
