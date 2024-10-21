#!/usr/bin/env python3
import gi
gi.require_version('Rsvg', '2.0')
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, GObject, GdkPixbuf, GLib, Gdk, Rsvg
from moviepy.editor import VideoFileClip
import os
import time
import threading
from PIL import Image
from basic_colormath import get_delta_e

if not hasattr(Image, 'ANTIALIAS'):
    Image.ANTIALIAS = Image.Resampling.LANCZOS  # Ensure compatibility with Pillow 10.x

if not Gtk.init_check():
    print("Failed to initialize GTK.")
    exit(1)

class framestop(Gtk.Window):

    def __init__(self):
        super().__init__(title="Framestop")
        self.set_border_width(10)
        self.set_default_size(800, 600)
        self.output_auto = True  # Automatically assign input folder to output folder by default
        self.frame_skip_value = 1
        self.frame_analysis_value = 5
        self.threshold = 5
        self.scale_factor = 1.0

        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=5)
        self.add(vbox)

        grid = Gtk.Grid()
        grid.set_column_homogeneous(False)
        grid.set_column_spacing(10)
        grid.set_row_spacing(10)
        vbox.pack_start(grid, True, True, 0)

        # Input file path
        input_label = Gtk.Label(label="Input File:")
        grid.attach(input_label, 0, 0, 1, 1)

        self.input_entry = Gtk.Entry(hexpand=True)
        grid.attach(self.input_entry, 1, 0, 1, 1)

        input_file_button = Gtk.Button(label="Select Input File")
        input_file_button.connect("clicked", self.on_select_input_file)
        grid.attach(input_file_button, 2, 0, 1, 1)

        # Output file path
        output_label = Gtk.Label(label="Output Folder:")
        grid.attach(output_label, 0, 1, 1, 1)

        self.output_entry = Gtk.Entry(hexpand=True)
        grid.attach(self.output_entry, 1, 1, 1, 1)

        output_directory_button = Gtk.Button(label="Select Output Folder")
        output_directory_button.connect("clicked", self.on_select_output_directory)
        grid.attach(output_directory_button, 2, 1, 1, 1)

        self.frame_area = Gtk.ScrolledWindow(hexpand=True, vexpand=True)
        self.frame_area.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        grid.attach(self.frame_area, 0, 2, 3, 1)

        # Adjustment for the slider
        adjustment = Gtk.Adjustment(value=0, lower=0, upper=0, step_increment=1, page_increment=1)
        self.frame_slider = Gtk.Scale(orientation=Gtk.Orientation.HORIZONTAL, adjustment=adjustment)
        self.frame_slider.set_digits(0)
        self.frame_slider.connect("value-changed", self.on_frame_slider_changed)

        # Horizontal box for the slider and arrow buttons
        hbox_slider = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)

        # Arrow button to skip frames backward
        self.removeframe_button = Gtk.Button()
        arrow_left = Gtk.Image.new_from_icon_name("go-previous", Gtk.IconSize.BUTTON)
        self.removeframe_button.set_image(arrow_left)
        self.removeframe_button.connect("clicked", self.on_remove_frame)

        # Arrow button to skip frames forward
        self.addframe_button = Gtk.Button()
        arrow_right = Gtk.Image.new_from_icon_name("go-next", Gtk.IconSize.BUTTON)
        self.addframe_button.set_image(arrow_right)
        self.addframe_button.connect("clicked", self.on_add_frame)

        # Adding components to hbox
        hbox_slider.pack_start(self.removeframe_button, False, False, 0)  # Left arrow button
        hbox_slider.pack_start(self.frame_slider, True, True, 0)          # Frame slider
        hbox_slider.pack_start(self.addframe_button, False, False, 0)     # Right arrow button

        # Attach hbox_slider to grid in place of the previous frame slider
        grid.attach(hbox_slider, 0, 3, 3, 1)

        # HBox to hold checkbox and other buttons
        hbox_controls = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        hbox_controls.set_halign(Gtk.Align.CENTER)  # Center align the hbox_controls

        # Checkbox for applying optimization
        self.optimize_checkbox = Gtk.CheckButton(label="Apply Optimization")
        self.optimize_checkbox.set_active(True)
        hbox_controls.pack_start(self.optimize_checkbox, False, False, 0)

        # Zoom in button
        zoom_in_button = Gtk.Button(label="Zoom In")
        zoom_in_button.connect("clicked", self.on_zoom_in)
        hbox_controls.pack_start(zoom_in_button, False, False, 0)

        # Zoom out button
        zoom_out_button = Gtk.Button(label="Zoom Out")
        zoom_out_button.connect("clicked", self.on_zoom_out)
        hbox_controls.pack_start(zoom_out_button, False, False, 0)

        # Clear all inputs button next to the frame buttons
        clearall_button = Gtk.Button(label="Clear all inputs")
        clearall_button.connect("clicked", self.clearall)
        hbox_controls.pack_start(clearall_button, False, False, 0)

        # Settings button 
        settings_button = Gtk.Button(label="Settings")
        settings_button.connect("clicked", self.on_open_settings)
        hbox_controls.pack_start(settings_button, False, False, 0)

        # About button 
        about_button = Gtk.Button(label="About")
        about_button.connect("clicked", self.on_about_button_clicked)
        hbox_controls.pack_start(about_button, False, False, 0)

        grid.attach(hbox_controls, 0, 4, 3, 1)

        self.status_label = Gtk.Label(label="")
        grid.attach(self.status_label, 0, 5, 3, 1)

        # second HBox to hold take screenshot & copy to clipboard
        hbox_controls2 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        hbox_controls2.set_halign(Gtk.Align.FILL)  # Center align the hbox_controls

        # Screenshot button
        screenshot_button = Gtk.Button(label="Take Screenshot")
        screenshot_button.connect("clicked", self.on_take_screenshot)
        hbox_controls2.pack_start(screenshot_button, True, True, 0)

        # Copy current frame to clipboard button next to the frame buttons
        copytoclip_button = Gtk.Button(label="Copy frame to clipboard")
        copytoclip_button.connect("clicked", self.copytoclip)
        hbox_controls2.pack_start(copytoclip_button, True, True, 0)                                                     

        grid.attach(hbox_controls2, 0, 6, 3, 1)

        self.frame_images = []
        self.current_frame = 0
        self.video = None
        self.pixbuf_cache = []
        self.loading_animation_id = None

        # Add loading label (for the "Loading frames..." message)
        self.loading_label = Gtk.Label(label="")
        grid.attach(self.loading_label, 0, 2, 3, 1)

        #a sidebar for further options and fine tuning adjustments could be included, consider this later
        
    def update_status(self, message):
        GLib.idle_add(self.status_label.set_text, message)

    def clearall(self,widget):  # This is meant to essentially bring the program back to its base state
        self.frame_images = []
        self.current_frame = 0
        self.video = None
        self.pixbuf_cache = []
        self.input_entry.set_text("")
        self.output_entry.set_text("")
        for child in self.frame_area.get_children():
            self.frame_area.remove(child)
        self.frame_slider.set_value(0)
        adjustment = self.frame_slider.get_adjustment()
        adjustment.set_lower(0)
        adjustment.set_upper(0)
        self.status_label.set_text("")
        self.stop_loading_animation()

    def copytoclip(self,widget):
        if not self.frame_images:
            self.show_error_dialog("Error: No frame to copy. Load a video first.")
            self.stop_loading_animation()
            return

        # Get the current frame image (PIL.Image object)
        selected_frame = self.frame_images[self.current_frame]
        if self.optimize_checkbox.get_active():
            selected_frame, self.current_frame = self.getBestFrame()

        # Convert the PIL Image to a GdkPixbuf object
        buffer = selected_frame.tobytes()
        pixbuf = GdkPixbuf.Pixbuf.new_from_data(
            buffer,
            GdkPixbuf.Colorspace.RGB,
            False, 8,
            selected_frame.width, selected_frame.height,
            selected_frame.width * 3
        )

        # Get the clipboard object and set the Pixbuf
        clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
        clipboard.set_image(pixbuf)

        # Update the status to inform the user
        self.update_status(f"Copied frame {self.current_frame} to clipboard.")
        print(f"Copied frame {self.current_frame} to clipboard.")

    def on_frame_skip_value_changed(self, widget):
        self.frame_skip_value = widget.get_value_as_int()
        if self.frame_skip_value == 1:
            self.removeframe_button.set_label(f"< - {self.frame_skip_value} frame")
            self.addframe_button.set_label(f"+ {self.frame_skip_value} frame >")
        else:
            self.removeframe_button.set_label(f"< - {self.frame_skip_value} frames")
            self.addframe_button.set_label(f"+ {self.frame_skip_value} frames >")
        

    def on_select_input_file(self, widget):
        self.clearall(widget) #isso é importante pra caso o usuário selecione um arquivo depois do outro
        dialog = Gtk.FileChooserDialog(
            title="Select Input File", parent=self, action=Gtk.FileChooserAction.OPEN
        )
        dialog.add_buttons(Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL, Gtk.STOCK_OPEN, Gtk.ResponseType.OK)
        response = dialog.run()
        if response == Gtk.ResponseType.OK:
            input_file_path = dialog.get_filename()
            self.input_entry.set_text(input_file_path)
            self.input_directory = os.path.dirname(input_file_path)
            self.start_loading_animation()
            # Load video frames in a separate thread to avoid freezing
            thread = threading.Thread(target=self.load_video_frames, args=(input_file_path,))
            thread.start()
            if self.output_auto:
                self.output_entry.set_text(self.input_directory)

        dialog.destroy()

    def start_loading_animation(self):
        self.loading_dots = 0

        def animate_loading():
            dots = "." * (self.loading_dots % 4)
            GLib.idle_add(self.loading_label.set_text, f"Loading frames{dots}")
            self.loading_dots += 1
            return True

        # Start the animation with a 500ms interval
        self.loading_animation_id = GLib.timeout_add(500, animate_loading)

    def stop_loading_animation(self):
        if self.loading_animation_id:
            GLib.source_remove(self.loading_animation_id)
            self.loading_animation_id = None
        self.loading_label.set_text("")  # Clear the loading label    

    def load_video_frames(self, input_file_path):
        time.sleep(5)  # Simulate a long operation; remove or modify this in real usage
        # Once loading is done, update the UI
        GLib.idle_add(self.on_frames_loaded)

    def load_video_frames(self, input_file):
        if not input_file:
            print("No input file selected.")
            self.stop_loading_animation()
            return
        try:
            self.video = VideoFileClip(input_file)
        except Exception as e:
            self.show_error_dialog("Error: Invalid video file selected.")
            self.stop_loading_animation()
            return
        
        self.frame_images.clear()
        self.pixbuf_cache.clear()

        for i, frame in enumerate(self.video.iter_frames()):
            image = Image.fromarray(frame)
            self.frame_images.append(image)

        self.frame_slider.get_adjustment().set_lower(0)
        self.frame_slider.get_adjustment().set_upper(len(self.frame_images) - 1)
        self.frame_slider.set_value(0)
        self.update_frame_display(0)

    def show_error_dialog(self, message):
        dialog = Gtk.MessageDialog(
            parent=self, flags=0, message_type=Gtk.MessageType.ERROR,
            buttons=Gtk.ButtonsType.OK, text="Input Error"
        )
        dialog.format_secondary_text(message)
        dialog.run()
        dialog.destroy()

    def on_select_output_directory(self, widget):
            dialog = Gtk.FileChooserDialog(
            title="Select Output Directory", parent=self, action=Gtk.FileChooserAction.SELECT_FOLDER
            )
            dialog.add_buttons(Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL, Gtk.STOCK_OPEN, Gtk.ResponseType.OK)
            response = dialog.run()
            if response == Gtk.ResponseType.OK:
                self.output_entry.set_text(dialog.get_filename())
            dialog.destroy()    

    def on_frame_slider_changed(self, widget):
        self.current_frame = int(self.frame_slider.get_value())
    
        # Get the current scroll positions (horizontal and vertical adjustments)
        hadjustment = self.frame_area.get_hadjustment()
        vadjustment = self.frame_area.get_vadjustment()
    
        hvalue = hadjustment.get_value()
        vvalue = vadjustment.get_value()

        # Update the frame display
        self.update_frame_display(self.current_frame)

        # Restore the scroll positions
        hadjustment.set_value(hvalue)
        vadjustment.set_value(vvalue)

    def update_frame_display(self, frame_index):
        if not self.frame_images:
            return

        frame_image = self.frame_images[frame_index]

        # Calculate the scaled width and height
        scaled_width = int(frame_image.width * self.scale_factor)
        scaled_height = int(frame_image.height * self.scale_factor)

        # Check if there's already a Gtk.Viewport inside the ScrolledWindow
        viewport = self.frame_area.get_child()  # Get the Gtk.Viewport inside the ScrolledWindow
        if viewport and viewport.get_children():
            # If a Gtk.Image exists inside the Viewport, update its pixbuf
            frame_image_widget = viewport.get_children()[0]
        else:
            # Create a new Gtk.Image if no widget exists yet
            frame_image_widget = Gtk.Image()
            self.frame_area.add(frame_image_widget)

        # Create or retrieve the Pixbuf, cache it if necessary
        if frame_index < len(self.pixbuf_cache):
            pixbuf = self.pixbuf_cache[frame_index]
        else:
            # Convert the frame_image to a GdkPixbuf object
            buffer = frame_image.tobytes()
            pixbuf = GdkPixbuf.Pixbuf.new_from_data(
                buffer,
                GdkPixbuf.Colorspace.RGB,
                False, 8,
                frame_image.width, frame_image.height,
                frame_image.width * 3
            )
            self.pixbuf_cache.append(pixbuf)

        # Scale the pixbuf to the new size
        scaled_pixbuf = pixbuf.scale_simple(scaled_width, scaled_height, GdkPixbuf.InterpType.BILINEAR)

        # Update the Gtk.Image widget with the new scaled pixbuf
        frame_image_widget.set_from_pixbuf(scaled_pixbuf)

        # Ensure everything is visible
        self.frame_area.show_all()

    def on_zoom_in(self, widget):
        """ Zoom in by increasing the scale factor and updating the display. """
        self.scale_factor *= 1.1
        self.update_frame_display(self.current_frame)

    def on_zoom_out(self, widget):
        """ Zoom out by decreasing the scale factor and updating the display. """
        self.scale_factor /= 1.1
        self.update_frame_display(self.current_frame)

    def on_window_resize(self, widget, allocation):
        """ Handle window resize and adjust frame size accordingly. """
        self.update_frame_display(self.current_frame)

    def on_take_screenshot(self, widget):
        if not self.frame_images:
            self.show_error_dialog("Error: Please select a proper video file.")
            return

        output_folder = self.output_entry.get_text()

        if not output_folder:
            self.show_error_dialog("Error: Please select a proper output folder.")
            return

        if not os.path.exists(output_folder):
            os.makedirs(output_folder)

        selected_frame = self.frame_images[self.current_frame]
        if self.optimize_checkbox.get_active():
            old_frame = self.current_frame
            self.update_status("Searching for the best frames...")
            selected_frame, self.current_frame = self.getBestFrame()
            selected_frame.save(os.path.join(output_folder, f"frame_{self.current_frame}.jpg"))
            self.update_status(f"Screenshot saved. Best frame at {self.current_frame}th frame ({abs(old_frame - self.current_frame)} frame{'s' if abs(old_frame - self.current_frame) != 1 else ''} away)")
            #TODO mover preview para o frame atual
        else:
            selected_frame.save(os.path.join(output_folder, f"frame_{self.current_frame}.jpg"))
            self.update_status(f"Screenshot of frame {self.current_frame} saved.")
            print(f"Screenshot of frame {self.current_frame} saved.")

    def neighbour_pixel_values(self,loaded_img, x,y):
        cima = loaded_img[x,y-1]
        direita = loaded_img[x+1, y]
        baixo = loaded_img[x, y+1]
        esquerda = loaded_img[x-1, y]
        return [cima, direita, baixo, esquerda]

    def imageRating(self,img):
        largura, altura = img.size
        loaded_img = img.load()
        mudancas = 0
        for linha in range(1, altura-1):
            for coluna in range(1, largura-1):
                if (linha%2 == 0 and coluna%2 == 0) or (linha%2 == 1 and coluna%2 == 1):
                    for pixel in self.neighbour_pixel_values(loaded_img, coluna, linha):
                        dE = get_delta_e(loaded_img[coluna, linha], pixel)
                        if dE > self.threshold:
                            mudancas += round(dE, 2)
        return mudancas*2/(largura*altura) #como estamos filtrando metade dos pixels, multiplicamos por 2
    
    def getBestFrame(self):
        #TODO melhorar range de frames quando está perto do início ou fim
        half_frames = self.frame_analysis_value // 2
        start_frame = int(max(0, self.current_frame - half_frames))
        end_frame = int(min(len(self.frame_images), self.current_frame + half_frames))
        frames_to_analyze_array = list(range(start_frame, end_frame))
        ratings = {}
        for frame_index in frames_to_analyze_array:
            copia = self.frame_images[frame_index].copy()
            copia.thumbnail((100,100))
            ratings[frame_index] = self.imageRating(copia)
        best_frame_index = max(ratings, key=ratings.get)
        return [self.frame_images[best_frame_index], best_frame_index]

    def on_add_frame(self, widget):
        # Move slider value forward by 1 frame
        current_value = self.frame_slider.get_value()
        self.frame_slider.set_value(min(current_value + self.frame_skip_value, len(self.frame_images) - 1))

    def on_remove_frame(self, widget):
        # Move slider value backward by 1 frame
        current_value = self.frame_slider.get_value()
        self.frame_slider.set_value(max(current_value - self.frame_skip_value, 0))

    def on_open_settings(self,widget):
        # this creates a dialog window for settings
        dialog = Gtk.Dialog(title="Settings", transient_for=self, flags=0)
        dialog.add_buttons(Gtk.STOCK_OK, Gtk.ResponseType.OK)

        # Get the content area of the dialog
        content_area = dialog.get_content_area()
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=15)
        vbox.set_halign(Gtk.Align.CENTER)  # Center align the hbox_controls
        vbox.set_margin_top(15)            # Add top margin
        vbox.set_margin_bottom(15)         # Add bottom margin
        vbox.set_margin_start(15)          # Add left margin
        vbox.set_margin_end(15)            # Add right margin
        content_area.pack_start(vbox, False, False, 0)
        dialog.set_default_size(420, 180)

        # Create the grid
        grid2 = Gtk.Grid()
        grid2.set_column_homogeneous(False)
        grid2.set_column_spacing(15)
        grid2.set_row_spacing(15)
        vbox.pack_start(grid2, True, True, 0)

        # Label for the output folder
        autoassignfoldercb_label = Gtk.Label(label="For saving Files:")
        grid2.attach(autoassignfoldercb_label, 0, 0, 1, 1)  # Left column (0), top row (0)

        # Checkbox to set output folder
        self.autoassignfoldercb = Gtk.CheckButton(label="Automatically set input folder as output folder")
        self.autoassignfoldercb.set_active(self.output_auto)
        self.autoassignfoldercb.connect("toggled", self.on_toggle_auto_output_folder)
        grid2.attach(self.autoassignfoldercb, 1, 0, 1, 1)  # Right column (1), same row (0)

        # Label for frame analysis range
        frame_analysis_label = Gtk.Label(label="Frame range for analysis:")
        grid2.attach(frame_analysis_label, 0, 1, 1, 1)  # Next row

        # SpinButton to set the frame analysis range
        self.frame_analysis_adj = Gtk.Adjustment(value=self.frame_analysis_value, lower=1, upper=100, step_increment=1, page_increment=10, page_size=0)
        self.frame_analysis_spin = Gtk.SpinButton(adjustment=self.frame_analysis_adj)
        self.frame_analysis_spin.connect("value-changed", self.on_frame_analysis_value_changed)

        self.frame_analysis_spin.set_halign(Gtk.Align.CENTER)  # Align to the right of the cell
        self.frame_analysis_spin.set_size_request(10,10)  # Adjust this value to control the width
        grid2.attach(self.frame_analysis_spin, 1, 1, 1, 1)  # Right column, same row as label (1)

        # Label for threshold range
        threshold_label = Gtk.Label(label="Frame selection Threshold:")
        grid2.attach(threshold_label, 0, 2, 1, 1)  

        # SpinButton to set the threshold range
        self.threshold_adj = Gtk.Adjustment(value=self.threshold, lower=1, upper=10, step_increment=1, page_increment=10, page_size=0)
        self.threshold_spin = Gtk.SpinButton(adjustment=self.threshold_adj)
        self.threshold_spin.connect("value-changed", self.on_threshold_value_changed)

        self.threshold_spin.set_halign(Gtk.Align.CENTER)  # Center it
        self.threshold_spin.set_size_request(100, 10)  # Adjust width as needed
        grid2.attach(self.threshold_spin, 1, 2, 1, 1)  # Attach the spin button

        # Label for threshold range
        FrameSkip_label = Gtk.Label(label="Skip X frames:")
        grid2.attach(FrameSkip_label, 0, 3, 1, 1) 

        #Skip X frames forward or backwards
        self.frame_skip_spinner_adj = Gtk.Adjustment(value=self.frame_skip_value, lower=1, upper=100, step_increment=1, page_increment=10, page_size=0)
        self.frame_skip_spinner = Gtk.SpinButton(adjustment=self.frame_skip_spinner_adj)
        self.frame_skip_spinner.connect("value-changed", self.on_frame_skip_value_changed)

        self.frame_skip_spinner.set_halign(Gtk.Align.CENTER)  # Center it
        self.frame_skip_spinner.set_size_request(100, 10)  # Adjust width as needed
        grid2.attach(self.frame_skip_spinner, 1, 3, 1, 1)  # Attach the spin button
        
        #TODO add the screenshot configurations here

        # Show the dialog with its contents
        dialog.show_all()

        # Wait for user response (OK or Cancel)
        response = dialog.run()

        if response == Gtk.ResponseType.OK:
            self.frame_analysis_value = self.frame_analysis_spin.get_value_as_int()
            self.threshold = self.threshold_spin.get_value_as_int()
            self.frame_skip_value=self.frame_skip_spinner.get_value_as_int()

        dialog.destroy()

    def on_about_button_clicked(self, widget):
         # Create the About dialog
        about_dialog = Gtk.Dialog(title="About Screenshot Optimizer", transient_for=self, flags=0)
        about_dialog.set_default_size(400, 400)  # Adjust size to fit the icon and text
        about_dialog.add_buttons(Gtk.STOCK_OK, Gtk.ResponseType.OK)

        # Create a vertical box to hold both the icon and the text
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        vbox.set_halign(Gtk.Align.CENTER)  # Center align the hbox_controls
        vbox.set_margin_top(15)            # Add top margin
        vbox.set_margin_bottom(15)         # Add bottom margin
        vbox.set_margin_start(15)          # Add left margin
        vbox.set_margin_end(15)            # Add right margin

        # Load the SVG icon
        icon_path = "/app/share/icons/hicolor/scalable/apps/io.github.Abstract_AA.Framestop.svg"  # Update with your SVG icon path
        try:
            handle = Rsvg.Handle.new_from_file(icon_path)  # Load the SVG file
            # Create a pixbuf from the SVG handle
            svg_dimensions = handle.get_dimensions()
            icon_pixbuf = handle.get_pixbuf()
        
            # Create an image from the pixbuf
            icon_image = Gtk.Image.new_from_pixbuf(icon_pixbuf)
            icon_image.set_halign(Gtk.Align.CENTER)  # Center the icon
            vbox.pack_start(icon_image, False, False, 0)
        except Exception as e:
            print(f"Error loading SVG: {e}")
            # Handle error if the SVG cannot be loaded
            icon_image = Gtk.Label(label="(Error loading SVG)")
            icon_image.set_halign(Gtk.Align.CENTER)
            vbox.pack_start(icon_image, False, False, 0)

        # Create a label with information about the program
        about_label = Gtk.Label(label=(
        "\n"
        "   This program is a frame capture tool that allows for the automatic selection of the clearer frame in a video or gif.  \n\n "
        "   Usage:\n   "
        "    1. Select an input video file.\n    "
        "   2. Set the frame skip value and optimization settings.\n    "
        "   3. Use the slider to navigate through frames and take screenshots.\n    "
        "   4. Copy frames to the clipboard or save them to the output folder.\n\n  "
        "   In the Settings menu, the threshold value represents how strict the frame selection will be, i.e. higher \n     threshold values mean that only very clear frames will be selected. \n\n "
        "   Version 1.1. This program comes with absolutely no warranty. Check the MIT Licence for further details.  "
        ))
        about_label.set_justify(Gtk.Justification.LEFT)
        about_label.set_halign(Gtk.Align.CENTER)

        # Add the label to the vertical box below the icon
        vbox.pack_start(about_label, True, True, 0)

        # Add the vbox to the content area of the dialog
        about_content_area = about_dialog.get_content_area()
        about_content_area.pack_start(vbox, True, True, 10)  # Add some padding for a cleaner look

        # Show all components
        about_dialog.show_all()

        # Run the dialog and wait for response
        about_dialog.run()
        about_dialog.destroy()


    def on_toggle_auto_output_folder(self, widget):
        self.output_auto = widget.get_active()
        print(f"Automatic output folder selection: {self.output_auto}")

    def on_frame_analysis_value_changed(self, widget):
        self.frame_analysis_value = widget.get_value()
        print(f"Frames to analyze: {self.frame_analysis_value}")
    
    def on_threshold_value_changed(self, widget):
        self.threshold_value = (widget.get_value())/10
        print(f"Threshold set at: {self.threshold_value}")

def main():
    app = framestop()
    app.connect("destroy", Gtk.main_quit)
    app.show_all()
    Gtk.main()

if __name__ == "__main__":
    main()
