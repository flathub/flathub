#!/usr/bin/env python3

"""crop_webtoon_gui.py

Simple PyQt6 GUI to slice tall webtoon images into equal parts
by height or by number of slices.

Run inside Flatpak or host:
    pip install PyQt6 pillow
    python crop_webtoon_gui.py
"""

import sys
import os
import math
from PIL import Image
from PyQt6.QtWidgets import (
    QApplication, QWidget, QLabel, QLineEdit, QPushButton, QFileDialog,
    QVBoxLayout, QHBoxLayout, QSpinBox, QMessageBox, QRadioButton
)
from PyQt6.QtCore import Qt

class CropWebtoonApp(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Manga Slicer")
        self.setFixedWidth(420)
        self.init_ui()

    def init_ui(self):
        path_label = QLabel("Image file:")
        self.path_edit = QLineEdit()
        self.path_edit.setPlaceholderText("Choose PNG/JPG webtoon page...")
        browse_btn = QPushButton("Browse")
        browse_btn.clicked.connect(self.browse_file)

        file_row = QHBoxLayout()
        file_row.addWidget(self.path_edit)
        file_row.addWidget(browse_btn)

        self.rb_height = QRadioButton("Slice by height (px)")
        self.rb_count = QRadioButton("Slice by number")
        self.rb_height.setChecked(True)

        self.spin_height = QSpinBox()
        self.spin_height.setRange(100, 10000)
        self.spin_height.setValue(1280)

        self.spin_count = QSpinBox()
        self.spin_count.setRange(1, 100)
        self.spin_count.setEnabled(False)

        self.rb_height.toggled.connect(self.toggle_mode)

        mode_height_row = QHBoxLayout()
        mode_height_row.addWidget(self.rb_height)
        mode_height_row.addStretch()
        mode_height_row.addWidget(self.spin_height)

        mode_count_row = QHBoxLayout()
        mode_count_row.addWidget(self.rb_count)
        mode_count_row.addStretch()
        mode_count_row.addWidget(self.spin_count)

        out_label = QLabel("Output folder (optional):")
        self.out_edit = QLineEdit()
        browse_out_btn = QPushButton("Browse")
        browse_out_btn.clicked.connect(self.browse_output)

        out_row = QHBoxLayout()
        out_row.addWidget(self.out_edit)
        out_row.addWidget(browse_out_btn)

        process_btn = QPushButton("Slice!")
        process_btn.clicked.connect(self.process_image)
        process_btn.setFixedHeight(40)

        layout = QVBoxLayout()
        layout.addWidget(path_label)
        layout.addLayout(file_row)
        layout.addSpacing(10)
        layout.addLayout(mode_height_row)
        layout.addLayout(mode_count_row)
        layout.addSpacing(10)
        layout.addWidget(out_label)
        layout.addLayout(out_row)
        layout.addSpacing(20)
        layout.addWidget(process_btn)
        self.setLayout(layout)

    def toggle_mode(self):
        height_mode = self.rb_height.isChecked()
        self.spin_height.setEnabled(height_mode)
        self.spin_count.setEnabled(not height_mode)

    def browse_file(self):
        file_path, _ = QFileDialog.getOpenFileName(self, "Select image", "", "Images (*.png *.jpg *.jpeg *.webp)")
        if file_path:
            self.path_edit.setText(file_path)

    def browse_output(self):
        folder = QFileDialog.getExistingDirectory(self, "Select output folder")
        if folder:
            self.out_edit.setText(folder)

    def process_image(self):
        img_path = self.path_edit.text().strip()
        if not img_path:
            QMessageBox.warning(self, "Missing file", "Please select an image file.")
            return
        if not os.path.exists(img_path):
            QMessageBox.critical(self, "File not found", "The specified image does not exist.")
            return

        try:
            img = Image.open(img_path)
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Cannot open image: {e}")
            return

        img_width, img_height = img.size

        if self.rb_height.isChecked():
            slice_height = self.spin_height.value()
            slices = math.ceil(img_height / slice_height)
        else:
            slices = self.spin_count.value()
            slice_height = math.ceil(img_height / slices)

        base_name = os.path.splitext(os.path.basename(img_path))[0]
        out_dir = self.out_edit.text().strip() or os.path.dirname(img_path)

        os.makedirs(out_dir, exist_ok=True)

        for i in range(slices):
            top = i * slice_height
            box = (0, top, img_width, min(top + slice_height, img_height))
            slice_img = img.crop(box)
            out_path = os.path.join(out_dir, f"{base_name}_part{i+1}.png")
            slice_img.save(out_path)

        QMessageBox.information(self, "Done", f"Sliced into {slices} parts.\nSaved to: {out_dir}")

def main():
    app = QApplication(sys.argv)
    win = CropWebtoonApp()
    win.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
