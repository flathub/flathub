# window.py
#
# Copyright 2026 masatn
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: GPL-3.0-or-later
import os
import gettext
import locale
import gi

locale.bindtextdomain("jp.co.masatn.ImageViewer", "/app/share/locale")
locale.textdomain("jp.co.masatn.ImageViewer")

gettext.bindtextdomain("jp.co.masatn.ImageViewer", "/app/share/locale")
gettext.textdomain("jp.co.masatn.ImageViewer")


from gettext import gettext as _


gi.require_version("Gtk", "4.0")
gi.require_version("Adw", "1")

from gi.repository import Gdk, Gtk, Adw, Gio, GObject, GdkPixbuf
from .shortcuts import ImageviewerShortcuts
from .viewer import ImageViewerDialog

IMAGE_EXTS = (".png", ".jpg", ".jpeg", ".gif", ".webp", ".svg")
THUMB = 128


@Gtk.Template(resource_path="/jp/co/masatn/ImageViewer/window.ui")
class ImageViewerWindow(Adw.ApplicationWindow):
    __gtype_name__ = "ImageViewerWindow"

    flowbox = Gtk.Template.Child()

    def __init__(self, app):
        super().__init__(application=app)

        self.app = app
        self.viewer = None

        # アクション
        shortcuts_action = Gio.SimpleAction.new("shortcuts", None)
        shortcuts_action.connect("activate", self.on_shortcuts)
        self.add_action(shortcuts_action)
        app.set_accels_for_action("win.shortcuts", ["<primary>question"])

        about_action = Gio.SimpleAction.new("about", None)
        about_action.connect("activate", self.on_about)
        self.add_action(about_action)
        app.set_accels_for_action("win.about", ["<primary>a"])

        self.connect("close-request", self.on_close_request)

        controller = Gtk.EventControllerKey()
        controller.connect("key-pressed", self.on_key_pressed)
        self.add_controller(controller)

    def on_key_pressed(self, controller, keyval, keycode, state):
        if keyval in (Gdk.KEY_space, Gdk.KEY_Return):
            self.open_selected_image()
            return True

        return False

    def open_selected_image(self):
        selected = self.flowbox.get_selected_children()

        if not selected:
            return

        child = selected[0]

        image_path = getattr(child, "image_path", None)

        if not image_path:
            return

        if self.viewer is not None:
            self.viewer.close()
            self.viewer = None

        viewer = ImageViewerDialog(self)
        viewer.open_image(image_path)
        viewer.connect("close-request", self.on_viewer_close)
        viewer.present()

        self.viewer = viewer

    def on_open(self, action, param):
        dialog = Gtk.FileDialog()

        dialog.select_folder(self, None, self.on_folder_selected)

    def on_folder_selected(self, dialog, result):

        try:
            folder = dialog.select_folder_finish(result)

        except Exception:
            return

        if folder:
            path = folder.get_path()

            if path:
                self.load_folder(path)

    def on_close_request(self, *args):
        self.get_application().quit()
        return False

    def on_about(self, action, param):
        builder = Gtk.Builder.new_from_resource("/jp/co/masatn/ImageViewer/about.ui")
        builder.set_translation_domain("jp.co.masatn.ImageViewer")

        about = builder.get_object("about")
        about.present(self)

    def on_shortcuts(self, action, param):
        win = ImageviewerShortcuts(self)
        win.present()

    def load_folder(self, path):
        while child := self.flowbox.get_first_child():
            self.flowbox.remove(child)

        for name in os.listdir(path):
            if not name.lower().endswith(IMAGE_EXTS):
                continue

            filepath = os.path.join(path, name)

            try:
                pixbuf = GdkPixbuf.Pixbuf.new_from_file_at_scale(
                    filepath, THUMB, THUMB, True
                )

                pic = Gtk.Picture.new_for_pixbuf(pixbuf)

                pic.set_size_request(THUMB, THUMB)
                pic.set_content_fit(Gtk.ContentFit.COVER)

                child = Gtk.FlowBoxChild()
                child.set_child(pic)

                child.image_path = filepath

                gesture = Gtk.GestureClick()

                def on_click(gesture, n, x, y, path=filepath):
                    if self.viewer is not None:
                        self.viewer.close()
                        self.viewer = None

                    viewer = ImageViewerDialog(self)
                    viewer.open_image(path)
                    viewer.connect("close-request", self.on_viewer_close)
                    viewer.present()

                    self.viewer = viewer

                gesture.connect("released", on_click)

                child.add_controller(gesture)

                # child = Gtk.FlowBoxChild()
                # child.set_child(pic)
                # child.image_path = filepath

                self.flowbox.append(child)

            except Exception as e:
                print("Failed to Read files:", filepath, e)

    def on_viewer_close(self, win):
        self.viewer = None
        return False
