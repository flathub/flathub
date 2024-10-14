#!/usr/bin/env python3

import os
import subprocess
import sys
import gi
gi.require_version("Gtk", "4.0")
from gi.repository import Gtk, GLib, Gdk
import threading

extensions = (".png", ".jpg", ".jpeg", ".tiff")

selected_images = []

libwebp = "cwebp"  

class WebPConverterWindow(Gtk.ApplicationWindow):
    def __init__(self, app):
        super().__init__(application=app)
        self.set_title("WebP Converter")
        self.set_default_size(400, 500)
        self.set_resizable(False)

        self.output_dir = os.path.join(os.path.expanduser("~"), "Pictures")
        if not os.path.exists(self.output_dir):
            os.makedirs(self.output_dir)

        self.images_selected = False

        #CSS custom styles
        css_provider = Gtk.CssProvider()
        css_provider.load_from_data(b'''
            button {
                padding: 5px 10px;
                border: none;
            }
            .wrap-label {
                max-width: 350px;
            }
            .wide-widget {
                max-width: 200px;
            }
        ''')
        Gtk.StyleContext.add_provider_for_display(
            Gdk.Display.get_default(), css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )

        #main vertical box container
        main_vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=20)
        main_vbox.set_margin_top(20)
        main_vbox.set_margin_bottom(20)
        main_vbox.set_margin_start(20)
        main_vbox.set_margin_end(20)
        self.set_child(main_vbox)

        #group 1: select Images to Convert
        images_group = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=5)
        images_group.set_hexpand(True)

        #button to select images or cancel
        self.select_images_button = Gtk.Button(label="Select Images")
        self.select_images_button.set_halign(Gtk.Align.CENTER)
        self.select_images_button.set_hexpand(False)
        self.select_images_button.set_vexpand(False)
        self.select_images_button.set_margin_top(10)
        self.select_images_button.connect("clicked", self.on_select_images_clicked)
        images_group.append(self.select_images_button)

        #label for selected images
        self.selected_images_label = Gtk.Label(label="No images selected.")
        self.selected_images_label.set_xalign(0.5)  # Center align the text
        self.selected_images_label.set_wrap(True)
        self.selected_images_label.set_max_width_chars(50)
        self.selected_images_label.set_margin_top(5)
        self.selected_images_label.get_style_context().add_class('wrap-label')
        images_group.append(self.selected_images_label)

        main_vbox.append(images_group)

        #group 2: select output directory
        output_group = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=5)
        output_group.set_hexpand(True)

        #button for output directory
        self.select_output_button = Gtk.Button(label="Output Directory")
        self.select_output_button.set_halign(Gtk.Align.CENTER)
        self.select_output_button.set_hexpand(False)
        self.select_output_button.set_vexpand(False)
        self.select_output_button.set_margin_top(10)
        self.select_output_button.connect("clicked", self.on_select_output_clicked)
        output_group.append(self.select_output_button)

        #label for selected output directory
        self.output_dir_label = Gtk.Label(label=f"Output Directory: {self.output_dir}")
        self.output_dir_label.set_xalign(0.5)
        self.output_dir_label.set_wrap(True)
        self.output_dir_label.set_max_width_chars(50)
        self.output_dir_label.set_margin_top(5)
        self.output_dir_label.get_style_context().add_class('wrap-label')
        output_group.append(self.output_dir_label)

        main_vbox.append(output_group)

        #group 3: image Quality
        quality_group = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=5)
        quality_group.set_hexpand(True)

        #label for image quality
        self.label = Gtk.Label(label="Select Image Quality (1-100):")
        self.label.set_xalign(0.5)
        self.label.set_margin_top(20)
        quality_group.append(self.label)

        #quality input
        self.scale = Gtk.Scale.new_with_range(Gtk.Orientation.HORIZONTAL, 1, 100, 1)
        self.scale.set_value(75)
        self.scale.set_digits(0)
        self.scale.set_hexpand(True)
        self.scale.connect("value-changed", self.on_scale_value_changed)
        self.scale.get_style_context().add_class('wide-widget')
        quality_group.append(self.scale)

        #label for current quality
        self.quality_label = Gtk.Label(label=f"Current Quality: {int(self.scale.get_value())}")
        self.quality_label.set_xalign(0.5)
        quality_group.append(self.quality_label)

        main_vbox.append(quality_group)

        #group 4: convert images and progress
        convert_group = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=5)
        convert_group.set_hexpand(True)

        #convert button
        self.button = Gtk.Button(label="Convert Images")
        self.button.set_halign(Gtk.Align.CENTER)
        self.button.set_hexpand(False)
        self.button.set_vexpand(False)
        self.button.set_margin_top(20)
        self.button.connect("clicked", self.on_convert_clicked)
        convert_group.append(self.button)

        #progress bar
        self.progress_bar = Gtk.ProgressBar()
        self.progress_bar.set_hexpand(True)
        self.progress_bar.set_vexpand(False)
        self.progress_bar.set_margin_top(20)  # More top margin from the button
        self.progress_bar.set_margin_bottom(0)  # Less bottom margin
        self.progress_bar.get_style_context().add_class('wide-widget')

        css_provider_pb = Gtk.CssProvider()
        css_provider_pb.load_from_data(b'''
            progressbar {
                min-height: 20px;
            }
        ''')
        style_context = self.progress_bar.get_style_context()
        style_context.add_provider(css_provider_pb, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

        convert_group.append(self.progress_bar)

        main_vbox.append(convert_group)

        #output
        self.output_label = Gtk.Label(label="")
        self.output_label.set_xalign(0.5)
        self.output_label.set_margin_top(10)
        main_vbox.append(self.output_label)

        self.dialog = None
        self.failed_images = []

    def on_select_images_clicked(self, widget):
        if self.images_selected:
            global selected_images
            selected_images = []
            self.selected_images_label.set_text("No images selected.")
            self.select_images_button.set_label("Select Images")
            self.images_selected = False
        else:
            self.dialog = Gtk.FileChooserNative(
                title="Select Images",
                transient_for=self,
                action=Gtk.FileChooserAction.OPEN,
                accept_label="Select",
                cancel_label="Cancel"
            )
            self.dialog.set_select_multiple(True)
            filter_images = Gtk.FileFilter()
            filter_images.set_name("Image files")
            for ext in extensions:
                filter_images.add_pattern(f"*{ext}")
            self.dialog.add_filter(filter_images)

            self.dialog.connect("response", self.on_file_dialog_response)
            self.dialog.show()

    def on_file_dialog_response(self, dialog, response):
        if response == Gtk.ResponseType.ACCEPT:
            files = dialog.get_files()
            selected_files = [f.get_path() for f in files]
            global selected_images
            selected_images = selected_files
            if selected_images:
                filenames = [os.path.basename(f) for f in selected_images]
                filenames_text = ", ".join(filenames)
                self.selected_images_label.set_text(filenames_text)
                self.select_images_button.set_label("Cancel")
                self.images_selected = True
            else:
                self.selected_images_label.set_text("No images selected.")
                self.select_images_button.set_label("Select Images")
                self.images_selected = False
        else:
            if not selected_images:
                self.selected_images_label.set_text("No images selected.")
                self.select_images_button.set_label("Select Images")
                self.images_selected = False
        dialog.destroy()
        self.dialog = None

    def on_select_output_clicked(self, widget):
        self.dialog = Gtk.FileChooserNative(
            title="Select Output Directory",
            transient_for=self,
            action=Gtk.FileChooserAction.SELECT_FOLDER,
            accept_label="Select",
            cancel_label="Cancel"
        )

        self.dialog.connect("response", self.on_output_dir_dialog_response)
        self.dialog.show()

    def on_output_dir_dialog_response(self, dialog, response):
        if response == Gtk.ResponseType.ACCEPT:
            folder = dialog.get_file()
            self.output_dir = folder.get_path()
            self.output_dir_label.set_text(f"Output Directory: {self.output_dir}")
        dialog.destroy()
        self.dialog = None

    def on_scale_value_changed(self, widget):
        current_value = int(widget.get_value())
        self.quality_label.set_text(f"Current Quality: {current_value}")

    def on_convert_clicked(self, widget):
        quality = str(int(self.scale.get_value()))
        output_dir = self.output_dir
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)

        if selected_images:
            self.failed_images = []
            self.button.set_sensitive(False)
            self.progress_bar.set_fraction(0.0)
            self.progress_bar.set_text("Starting conversion...")
            threading.Thread(target=self.convert_images, args=(selected_images.copy(), quality, output_dir)).start()
        else:
            self.output_label.set_text("No images selected for conversion.")

    def convert_images(self, images, quality, output_dir):
        total_images = len(images)
        for index, image in enumerate(images):
            input_file = image
            image_name = os.path.basename(image)
            output_file = os.path.join(output_dir, os.path.splitext(image_name)[0] + ".webp")
            try:
                result = subprocess.call(
                    [libwebp, "-quiet", "-q", quality, input_file, "-o", output_file],
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL
                )
                if result != 0:
                    self.failed_images.append(image_name)
            except Exception as e:
                print(e)
                self.failed_images.append(image_name)
            fraction = (index + 1) / total_images
            GLib.idle_add(self.progress_bar.set_fraction, fraction)
            GLib.idle_add(self.progress_bar.set_text, f"Converting... {int(fraction * 100)}%")
        GLib.idle_add(self.conversion_complete)

    def conversion_complete(self):
        total_images = len(selected_images)
        failed = len(self.failed_images)
        converted = total_images - failed
        self.output_label.set_text(f"Converted {converted} of {total_images} images. Failed: {failed}")
        if failed > 0:
            error_message = "\n".join(self.failed_images)
            dialog = Gtk.MessageDialog(
                transient_for=self,
                modal=True,
                message_type=Gtk.MessageType.ERROR,
                buttons=Gtk.ButtonsType.OK,
                text="Failed to convert the following images:",
                secondary_text=error_message
            )
            dialog.connect("response", lambda d, r: d.destroy())
            dialog.show()
        self.progress_bar.set_fraction(1.0)
        self.progress_bar.set_text("Conversion complete.")
        self.button.set_sensitive(True)

class WebPConverterApp(Gtk.Application):
    def __init__(self):
        super().__init__(application_id='com.example.WebPConverter')

    def do_activate(self):
        win = WebPConverterWindow(self)
        win.present()

def main():
    app = WebPConverterApp()
    app.run(sys.argv)

if __name__ == "__main__":
    main()
