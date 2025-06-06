from gi.repository import GObject, GLib
from dataclasses import dataclass
import os

CACHE_DIR = os.path.join(GLib.get_user_cache_dir(), "wselector")

@dataclass
class WallpaperInfo:
    """Data class for wallpaper information."""
    id: str
    url: str
    thumbnail_url: str
    title: str
    category: str
    purity: str

    def get_cached_path(self):
        """Returns the local cache path for the thumbnail."""
        return os.path.join(CACHE_DIR, f"{self.id}.jpg")

    def is_cached(self):
        """Checks if the thumbnail is cached locally."""
        return os.path.exists(self.get_cached_path())

class WallpaperGObject(GObject.Object):
    """GObject wrapper for WallpaperInfo."""
    __gtype_name__ = 'WallpaperGObject'
    
    id = GObject.Property(type=str)
    url = GObject.Property(type=str)
    thumbnail_url = GObject.Property(type=str)
    title = GObject.Property(type=str)
    category = GObject.Property(type=str)
    purity = GObject.Property(type=str)
    
    def __init__(self, wallpaper_info: WallpaperInfo):
        GObject.Object.__init__(self)
        self.id = wallpaper_info.id
        self.url = wallpaper_info.url
        self.thumbnail_url = wallpaper_info.thumbnail_url
        self.title = wallpaper_info.title
        self.category = wallpaper_info.category
        self.purity = wallpaper_info.purity
