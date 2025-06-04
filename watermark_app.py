#!/usr/bin/python3
# antoine@ginies.org
# GPL2

import tkinter as tk
from tkinter import filedialog, messagebox
import random
import time
import os
from PIL import Image, ImageDraw, ImageFont, ImageTk

class CustomMessageDialog(tk.Toplevel):
    def __init__(self, title=None, message="", parent=None):
        super().__init__(parent)
        self.title(title)
        # Create a frame for the dialog content
        self.frame = tk.Frame(self)
        self.frame.pack(padx=10, pady=10)

        self.message_label = tk.Label(self.frame, text=message, wraplength=400, justify='left')
        self.message_label.pack()

        self.ok_button = tk.Button(self.frame, text="OK", command=self.destroy)
        self.ok_button.pack(pady=10)

        self.transient(parent)
        if parent is not None:
            self.wait_window(self)

class FiligraneApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Watermark/Filigrane App")
        #self.root.resizable(False, False)
        self.output_folder_path = ""
        self.selected_files_path = []
        self.compression_rate = 75
        self.fili_font_size = 20
        self.fili_density = 40
        self.all_images = []
        self.current_image_index = 0
        self.image_paths = ""

        # Main frame for all widgets
        main_frame = tk.Frame(root, padx=10, pady=10) #, width=100, height=100)
        main_frame.pack(fill="both", expand=True)

        # Create a menu bar
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        menubar.add_command(label="Exit", command=self.root.quit)
        about_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="About", menu=about_menu)
        about_menu.add_command(label="About Filigrane App", command=self.about_dialog)

        # File selection frame
        file_frame = tk.Frame(main_frame)
        file_frame.pack(fill="x", pady=5)
        tk.Label(file_frame, text="Select Image(s)").pack(side="left", padx=(0, 10))
        self.file_button = tk.Button(file_frame, text="Select", command=self.select_file)
        self.file_button.pack(side="right")

        self.files_label = tk.Label(main_frame, text="", wraplength=400, justify="left")
        self.files_label.pack(expand=True, fill="x")

        # Filigrane text frame
        filigrane_frame = tk.Frame(main_frame)
        filigrane_frame.pack(fill="x", pady=5)
        tk.Label(filigrane_frame, text="Watermark/Filigrane Text").pack(side="left", padx=(0, 10))
        self.filigrane_entry = tk.Entry(filigrane_frame)
        self.filigrane_entry.insert(0, "Data")
        self.filigrane_entry.pack(side="left", expand=True, fill="x", padx=(0, 5))
        tk.Label(filigrane_frame, text="+ Date_Time").pack(side="right")

        # Output path selection frame
        output_frame = tk.Frame(main_frame)
        output_frame.pack(fill="x", pady=5)
        tk.Label(output_frame, text="Backup Directory (to store Watermaked images)").pack(side="left", padx=(0, 10))
        self.output_button = tk.Button(output_frame, text="Select", command=self.select_output_folder)
        self.output_button.pack(side="right")

        # Expert mode toggle
        #self.expert_mode_var = tk.BooleanVar(value=False)
        #self.expert_mode_checkbox = tk.Checkbutton(main_frame, text="Expert Mode", variable=self.expert_mode_var, command=self.toggle_expert_mode)

        # Add filigrane button
        self.add_filigrane_button = tk.Button(main_frame, text="Add Watermark/Filigrane", command=self.on_add_filigrane_clicked)
        self.add_filigrane_button.pack(side="top")#, pady=(5, 5))

        # Generated file label
        self.generated_file_label = tk.Label(main_frame, text="")
        self.generated_file_label.pack(pady=5)

        # Image display label
        self.image_label = tk.Label(main_frame)
        self.image_label.pack(pady=(5, 5))

    def display_single_image(self, image_path):
        img = Image.open(image_path)
        img.thumbnail((800, 600), Image.LANCZOS)
        tk_img = ImageTk.PhotoImage(img)
        if hasattr(self, 'image_label') and self.image_label is not None:
            self.image_label.destroy()
        self.image_label = tk.Label(self.root, image=tk_img)
        self.image_label.image = tk_img
        self.image_label.pack()

    def display_multiple_images(self, image_paths):
        """ displaying multiple images """
        for widget in self.root.winfo_children():
            if isinstance(widget, tk.Canvas) or isinstance(widget, tk.Label):
                widget.destroy()

        self.image_paths = image_paths
        self.filename_label = tk.Label(self.root, text="")
        self.filename_label.pack(pady=5)

        self.image_label = tk.Label(self.root, text="No image(s) Selected")
        self.image_label.pack(pady=10, padx=10)

        next_button = tk.Button(self.root, text=" > ", command=self.next_image)
        prev_button = tk.Button(self.root, text=" < ", command=self.prev_image)

        self.count_label = tk.Label(self.root, text=f"Image 1 / {len(image_paths)}")
        self.count_label.pack(side=tk.BOTTOM, pady=5)

        next_button.pack(side='right', padx=5)
        prev_button.pack(side='left', padx=5)
        self.count_label.place(relx=0.5, rely=1, anchor="s", x=0)
        self.show_image(self.current_image_index)

    def next_image(self):
        """ got to next image """
        if len(self.image_paths) > 0:
            new_index = (self.current_image_index + 1) % len(self.image_paths)
            self.current_image_index = new_index
            self.show_image(new_index)

    def prev_image(self):
        """ got to previous image """
        if len(self.image_paths) > 0:
            new_index = (self.current_image_index - 1) % len(self.image_paths)
            self.current_image_index = new_index
            self.show_image(new_index)

    def show_image(self, index):
        """ show the image """
        if 0 <= index < len(self.image_paths):
            img = Image.open(self.image_paths[index])
            img.thumbnail((800, 600), Image.LANCZOS)
            tk_img = ImageTk.PhotoImage(img)

            self.image_label.config(image=tk_img)
            self.image_label.image = tk_img

        self.count_label.config(text=f"Image {index + 1} of {len(self.image_paths)}")
        filename = os.path.basename(self.image_paths[index])
        print(f"Updating filename label with: {filename}")
        self.filename_label.config(text=filename)

    def select_file(self):
        """ select dialog"""
        filetypes = [
            ("All files", "*.*"),
            ("PNG files", "*.png"),
            ("JPEG files", "*.jpg;*.jpeg"),
            ("GIF files", "*.gif"),
            ("BMP files", "*.bmp"),
        ]

        files = filedialog.askopenfilenames(title="Select File(s)", filetypes=filetypes)
        if files:
            self.selected_files_path = list(files)
            self.update_file_button_text()
            self.current_image_index = 0
            print(f"Selected file(s): {self.selected_files_path}")

    def update_file_button_text(self):
        selected_files_str = ", ".join(os.path.basename(path) for path in self.selected_files_path)
        self.files_label.config(text=f"Selected File(s):\n {selected_files_str}")

    def select_output_folder(self):
        folder_path = filedialog.askdirectory(title="Choose Backup Directory")
        if folder_path:
            self.output_button.config(text=os.path.basename(folder_path))
            self.output_folder_path = folder_path

    def toggle_expert_mode(self):
        """Show or hide the expert mode settings."""
        if self.expert_mode_var.get():
            self.show_expert_settings()
        else:
            self.hide_expert_settings()

    def show_expert_settings(self):
        """Display the additional settings for expert mode."""
        # Compression rate
        tk.Label(self.root, text="JPEG Compression (0=none; 95=Strong)").grid(row=5, column=0, padx=10, pady=5, sticky='e')
        self.compression_entry = tk.Entry(self.root)
        self.compression_entry.insert(0, str(self.compression_rate))
        self.compression_entry.grid(row=5, column=1, padx=10, pady=5)

        # Fili Density
        #tk.Label(self.root, text="Densité (5-20):").grid(row=6, column=0, padx=10, pady=5, sticky='e')
        #self.density_entry = tk.Entry(self.root)
        #self.density_entry.insert(0, str(self.fili_density))
        #self.density_entry.grid(row=6, column=1, padx=10, pady=5)

    def hide_expert_settings(self):
        """Hide the additional settings for expert mode."""
        # Destroy labels and entries to hide them
        try:
            # Compression rate label and entry
            self.root.grid_slaves(row=5, column=0)[0].destroy()
            self.compression_entry.destroy()
        except (IndexError, AttributeError):
            pass

    def about_dialog(self):
        # Create a custom dialog window for the About section with a clickable link
        about_window = tk.Toplevel(self.root)
        about_window.title("About Watermark/Filigrane App")
        about_window.resizable(False, False)
        info_text = (
            "Watermark/Filigrane App Version 2.0\n\n"
            "This app add a Watermark/Filigrane to images\n"
            "Licence GPL2 \n\n"
            "Project Open Source on GitHub: "
        )

        label = tk.Label(about_window, text=info_text)
        label.pack(padx=20, pady=10)

        # Create a clickable hyperlink
        github_link = tk.Label(about_window, text="https://github.com/aginies/watermark", foreground="blue", cursor="hand2")
        github_link.pack(padx=20, pady=5)
        github_link.bind("<Button-1>", lambda e: self.open_github())

        close_button = tk.Button(about_window, text="Close", command=about_window.destroy)
        close_button.pack(pady=10)

    def open_github(self):
        """ open github web """
        # Open the GitHub link in the default web browser
        import webbrowser
        webbrowser.open("https://github.com/aginies/watermark")

    def on_add_filigrane_clicked(self):
        """ action when clic! """
        self.current_image_index = 0
        if not self.selected_files_path:
            messagebox.showwarning("Input Error", "Please select an Image")
            return

        filigrane_text = self.filigrane_entry.get()

        if not filigrane_text:
            messagebox.showwarning("Input Error", "Please enter a watermark/Filigrane text")
            return

        if not self.output_folder_path:
            default_output_dir = os.path.dirname(self.selected_files_path[0])
            self.output_button.config(text=default_output_dir)
        else:
            default_output_dir = self.output_folder_path

        try:
            for image_path in self.selected_files_path:
                output_image_path = self.add_filigrane_to_image(image_path, filigrane_text)
                if output_image_path:
                    print("Success", f"Generated File: {os.path.basename(output_image_path)}\n")
                    self.all_images.append(output_image_path)

            if len(self.selected_files_path) == 1:
                self.display_single_image(self.all_images[0])
                self.all_images = []
            else:
                self.display_multiple_images(self.all_images)
                self.all_images = []

        except Exception as err:
            print(f"Error processing the image: {err}")
            messagebox.showerror("Processing Error", f"An error occurred during image processing: {err}")

    def get_current_time_ces(self):
        now = time.time()
        cest_time = time.localtime(now + 3600)
        return cest_time

    def add_filigrane_to_image(self, image_path, text):
        try:
            with Image.open(image_path).convert("RGBA") as img:
                # Resize image if it's too large while preserving aspect ratio
                width_percent = (1280 / float(img.width))
                height_size = int((float(img.height) * float(width_percent)))

                if max(img.size) > 1280:
                    img = img.resize((1280, height_size), Image.LANCZOS)

                draw = ImageDraw.Draw(img)
                cest_time = self.get_current_time_ces()

                # Load font
                if os.path.exists('DejaVuSans.ttf'):
                    font = ImageFont.truetype("DejaVuSans.ttf", self.fili_font_size)
                else:
                    print("No DejaVuSans Font....")
                    font = ImageFont.truetype("arial.ttf", self.fili_font_size)
    
                timestamp_str_text = time.strftime('%d%m%Y_%H%M%S', cest_time)
                full_filigrane_text = f"{text} {timestamp_str_text}"
                bbox = draw.textbbox((0, 0), full_filigrane_text, font=font)

                text_width = bbox[2] - bbox[0]
                text_height = bbox[3] - bbox[1]

                dpi = self.fili_density
                interval_pixels_y = int(dpi)
                used_positions = set()

                # Draw the continuous watermark across the image
                for y in range(interval_pixels_y, img.height, interval_pixels_y):
                    x_positions = [(x % img.width) for x in range(0, img.width, text_width)]

                    for x in x_positions:
                        if (x, y) not in used_positions:
                            angle = random.uniform(-30, 30)
                            color = (
                                random.randint(0, 255),
                                random.randint(0, 255),
                                random.randint(0, 255),
                                50 # Adjust transparency
                            )
                            rotated_text_img = Image.new('RGBA', img.size)
                            draw_rotated = ImageDraw.Draw(rotated_text_img)
                            text_position = (x + text_width / 2, y + text_height / 2)
                            temp_image = Image.new('RGBA', (text_width * 3, text_height * 3), (0, 0, 0, 0))
                            temp_draw = ImageDraw.Draw(temp_image)
                            temp_draw.text((text_width, text_height), full_filigrane_text, font=font, fill=color)
                            rotated_text = temp_image.rotate(angle, expand=True)
                            rotated_text_position = (
                                x + text_width / 2 - rotated_text.width / 2,
                                y + text_height / 2 - rotated_text.height / 2
                            )
                            img.paste(rotated_text, (int(rotated_text_position[0]), int(rotated_text_position[1])), mask=rotated_text)
                            used_positions.add((x, y))

                timestamp_str = time.strftime('%d%m%Y_%H%M%S', cest_time)
                original_filename = os.path.basename(image_path)
                name_without_ext = os.path.splitext(original_filename)[0]
                final_filename = f"ready_{text}_{timestamp_str}_{name_without_ext}.jpg"
                output_path = os.path.join(self.output_folder_path, final_filename)
                img.convert("RGB").save(output_path, "JPEG", quality=self.compression_rate)
                return output_path

        except Exception as err:
            print(f"Error adding filigrane: {err}")
            messagebox.showerror("Filigrane Error", f"An error occurred while adding the filigrane: {err}")
            return None

if __name__ == "__main__":
    ROOT = tk.Tk()
    APP = FiligraneApp(ROOT)
    ROOT.mainloop()
