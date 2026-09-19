#!/usr/bin/env python3
import sys
import os
import shutil
from datetime import datetime
from PIL import Image, ExifTags, ImageQt

from PySide6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QPushButton, QCheckBox, QTableView, QHeaderView,
    QGraphicsView, QGraphicsScene,
    QFileDialog, QComboBox, QGroupBox,
    QMessageBox, QGridLayout, QSizePolicy
)
from PySide6.QtGui import (
    QPixmap, QIcon, QStandardItemModel, QStandardItem, QDesktopServices,
    QColor, QPainter, QFont, QPalette, QAction, QBrush
)
from PySide6.QtCore import Qt, QUrl

APP_NAME = "Photo Organizer"
VERSION = "1.0.0"
AUTHOR = "Giuseppe Cigala"


def resource_path(filename):

    if getattr(sys, 'frozen', False):
        base_path = getattr(sys, '_MEIPASS', os.path.dirname(sys.executable))
    else:
        base_path = os.path.dirname(os.path.abspath(__file__))
    return os.path.join(base_path, 'resources', filename)


class Photo:
    def __init__(self, source_path: str, date_time: str = "", width: int = 0, height: int = 0,
                 dpi: int = 0, depth: str = "", color_model: str = "",
                 latitude: float = 0.0, longitude: float = 0.0):
        self.source_path = source_path
        self.date_time = date_time
        self.width = width
        self.height = height
        self.dpi = dpi
        self.depth = depth
        self.color_model = color_model
        self.latitude = latitude
        self.longitude = longitude

    def __repr__(self):
        return f"Photo(path='{self.source_path}', date='{self.date_time}')"


class PhotoOrganizerApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Photo Organizer")

        self.setMinimumSize(700, 720)
        self.setMaximumHeight(720)

        self.photos_list = []
        self.current_selected_photo = None
        self.source_directory = ""
        self.destination_directory = ""

        self.initUi()
        self.setDarkTheme()

    def initUi(self):
        self.createMenuBar()

        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)
        self.main_layout = QVBoxLayout(self.central_widget)
        self.main_layout.setSpacing(10)

        self.content_layout = QHBoxLayout()
        self.main_layout.addLayout(self.content_layout)

        self.createPhotosPanel()
        self.content_layout.addWidget(self.photos_panel, 1)

        self.createDetailsPanel()
        self.content_layout.addWidget(self.details_panel, 0)

        self.createSourceSections()
        self.createDestinationSections()
        self.main_layout.addLayout(self.source_layout)
        self.main_layout.addLayout(self.destination_layout)

        self.createToolsSection()
        self.main_layout.addWidget(self.tools_groupbox)

        try:
            self.app_icon = QIcon(self.get_icon_path("app_icon"))
            if self.app_icon.isNull():
                svg_path = resource_path("app_icon.svg")
                if os.path.exists(svg_path):
                    self.app_icon = QIcon(svg_path)
            self.setWindowIcon(self.app_icon)
        except Exception:
            self.app_icon = QIcon()

        if not self.app_icon.isNull():
            QApplication.instance().setWindowIcon(self.app_icon)

    def createMenuBar(self):
        menubar = self.menuBar()

        help_menu = menubar.addMenu("Help")

        about_action = QAction("About", self)
        about_action.triggered.connect(self.showAboutDialog)
        help_menu.addAction(about_action)

        license_action = QAction("License Information", self)
        license_action.triggered.connect(self.showLicenseDialog)
        help_menu.addAction(license_action)

    def createPhotosPanel(self):
        self.photos_panel = QGroupBox("Photos")
        self.photos_panel_layout = QVBoxLayout(self.photos_panel)
        self.photos_panel_layout.setContentsMargins(10, 10, 10, 10)

        self.photos_panel.setMaximumHeight(500)
        self.photos_panel.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

        self.photos_table_model = QStandardItemModel()
        self.photos_table_model.setHorizontalHeaderLabels(["File Name", "Date - Time"])
        self.photos_table_view = QTableView()
        self.photos_table_view.setModel(self.photos_table_model)
        self.photos_table_view.setSelectionBehavior(QTableView.SelectRows)
        self.photos_table_view.setSelectionMode(QTableView.ExtendedSelection)

        self.photos_table_view.horizontalHeader().setSectionResizeMode(QHeaderView.Interactive)
        self.photos_table_view.horizontalHeader().setStretchLastSection(True)
        self.photos_table_view.horizontalHeader().setSectionsMovable(True)
        self.photos_table_view.doubleClicked.connect(self.openPreview)
        self.photos_table_view.selectionModel().selectionChanged.connect(self.onPhotoSelected)
        self.photos_table_view.verticalHeader().setDefaultSectionSize(22)

        self.photos_panel_layout.addWidget(self.photos_table_view)

    def createDetailsPanel(self):
        self.details_panel = QGroupBox("Details")
        self.details_panel_layout = QVBoxLayout(self.details_panel)
        self.details_panel_layout.setContentsMargins(10, 10, 10, 10)

        self.details_panel.setFixedWidth(300)
        self.details_panel.setMaximumHeight(500)
        self.details_panel.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)

        self.image_scene = QGraphicsScene()
        self.preview_iv = QGraphicsView(self.image_scene)
        self.preview_iv.setAlignment(Qt.AlignCenter)
        self.preview_iv.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.preview_iv.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.preview_iv.setRenderHint(QPainter.Antialiasing)

        self.preview_iv.setFixedSize(260, 260)
        self.details_panel_layout.addWidget(self.preview_iv, alignment=Qt.AlignHCenter)

        self.details_panel_layout.addSpacing(15)

        self.metadata_grid = QGridLayout()
        self.metadata_grid.setHorizontalSpacing(4)
        self.metadata_grid.setVerticalSpacing(5)

        font = QFont()
        font.setPointSize(10)

        bold_font = QFont()
        bold_font.setPointSize(10)
        bold_font.setBold(True)

        def make_label(text, align):
            lbl = QLabel(text)
            lbl.setFont(font)
            lbl.setAlignment(align)
            return lbl

        def make_value_label(align):
            lbl = QLabel("")
            lbl.setFont(bold_font)
            lbl.setAlignment(align)
            return lbl

        self.width_lbl = make_label("Width:", Qt.AlignLeft | Qt.AlignVCenter)
        self.width_val = make_value_label(Qt.AlignRight | Qt.AlignVCenter)
        self.height_lbl = make_label("Height:", Qt.AlignLeft | Qt.AlignVCenter)
        self.height_val = make_value_label(Qt.AlignRight | Qt.AlignVCenter)
        self.dpi_lbl = make_label("DPI:", Qt.AlignLeft | Qt.AlignVCenter)
        self.dpi_val = make_value_label(Qt.AlignRight | Qt.AlignVCenter)
        self.depth_lbl = make_label("Depth:", Qt.AlignLeft | Qt.AlignVCenter)
        self.depth_val = make_value_label(Qt.AlignRight | Qt.AlignVCenter)
        self.color_lbl = make_label("Color:", Qt.AlignLeft | Qt.AlignVCenter)
        self.color_val = make_value_label(Qt.AlignRight | Qt.AlignVCenter)

        self.metadata_grid.addWidget(self.width_lbl, 0, 0)
        self.metadata_grid.addWidget(self.width_val, 0, 1)
        self.metadata_grid.addWidget(self.height_lbl, 1, 0)
        self.metadata_grid.addWidget(self.height_val, 1, 1)
        self.metadata_grid.addWidget(self.dpi_lbl, 2, 0)
        self.metadata_grid.addWidget(self.dpi_val, 2, 1)
        self.metadata_grid.addWidget(self.depth_lbl, 3, 0)
        self.metadata_grid.addWidget(self.depth_val, 3, 1)
        self.metadata_grid.addWidget(self.color_lbl, 4, 0)
        self.metadata_grid.addWidget(self.color_val, 4, 1)

        self.metadata_grid.setColumnStretch(1, 1)

        metadata_widget = QWidget()
        metadata_widget.setLayout(self.metadata_grid)
        metadata_widget.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Fixed)
        self.details_panel_layout.addWidget(metadata_widget)

        self.details_panel_layout.addStretch(1)

    def updateMetadataLabel(self, label_widget: QLabel, value: str):
        label_widget.setText(value)

    def createSourceSections(self):
        self.source_groupbox = QGroupBox("Source")
        self.source_groupbox.setFixedHeight(70)
        self.source_layout = QVBoxLayout()
        self.source_layout.setSpacing(10)

        source_layout = QHBoxLayout(self.source_groupbox)
        self.source_path_control = QLabel("No source selected")

        bold_path_font = QFont()
        bold_path_font.setBold(True)
        self.source_path_control.setFont(bold_path_font)

        source_layout.addWidget(self.source_path_control)
        source_layout.addStretch()
        self.select_source_btn = QPushButton("Select Source")
        self.select_source_btn.setToolTip("Select the source directory for photos")
        self.select_source_btn.clicked.connect(self.selectSourceDirectory)
        source_layout.addWidget(self.select_source_btn)

        self.source_layout.addWidget(self.source_groupbox)

    def createDestinationSections(self):
        self.destination_groupbox = QGroupBox("Destination")
        self.destination_groupbox.setFixedHeight(70)
        self.destination_layout = QVBoxLayout()
        self.destination_layout.setSpacing(10)

        destination_layout = QHBoxLayout(self.destination_groupbox)
        self.destination_path_control = QLabel("No destination selected")

        bold_path_font = QFont()
        bold_path_font.setBold(True)
        self.destination_path_control.setFont(bold_path_font)

        destination_layout.addWidget(self.destination_path_control)
        destination_layout.addStretch()
        self.select_destination_btn = QPushButton("Select Destination")
        self.select_destination_btn.setToolTip("Select the destination directory for organized photos")
        self.select_destination_btn.clicked.connect(self.selectDestinationDirectory)
        destination_layout.addWidget(self.select_destination_btn)

        self.destination_layout.addWidget(self.destination_groupbox)

    def createToolsSection(self):
        self.tools_groupbox = QGroupBox("Tools")
        self.tools_groupbox.setFixedHeight(70)
        tools_layout = QHBoxLayout(self.tools_groupbox)

        structure_label = QLabel("Structure:")
        tools_layout.addWidget(structure_label)
        self.structure_combobox = QComboBox()
        self.structure_combobox.addItems(["yyyy", "yyyy/mm", "yyyy/mm/dd"])
        self.structure_combobox.setCurrentText("yyyy/mm/dd")
        tools_layout.addWidget(self.structure_combobox)

        self.rename_by_date_swt = QCheckBox("Rename by Date")
        self.rename_by_date_swt.setChecked(False)
        self.rename_by_date_swt.setToolTip(
            "If checked, the file is renamed using its Date - Time "
            "(e.g. 2024-03-15_14-30-22.jpeg). Otherwise the original name is kept."
        )
        tools_layout.addWidget(self.rename_by_date_swt)

        tools_layout.addStretch()

        self.organize_photos_btn = QPushButton("Organize Photos")
        self.organize_photos_btn.setToolTip("Organize photos into the destination directory")
        self.organize_photos_btn.clicked.connect(self.organizePhotos)
        tools_layout.addWidget(self.organize_photos_btn)

        self.tools_groupbox.setLayout(tools_layout)

    def setDarkTheme(self):
        dark_palette = QPalette()
        dark_palette.setColor(QPalette.Window, QColor(43, 43, 43))
        dark_palette.setColor(QPalette.WindowText, Qt.white)
        dark_palette.setColor(QPalette.Base, QColor(30, 30, 30))
        dark_palette.setColor(QPalette.AlternateBase, QColor(43, 43, 43))
        dark_palette.setColor(QPalette.ToolTipBase, QColor(60, 60, 60))
        dark_palette.setColor(QPalette.ToolTipText, Qt.white)
        dark_palette.setColor(QPalette.Text, Qt.white)
        dark_palette.setColor(QPalette.Button, QColor(55, 55, 55))
        dark_palette.setColor(QPalette.ButtonText, Qt.white)
        dark_palette.setColor(QPalette.Mid, QColor(70, 70, 70))
        dark_palette.setColor(QPalette.BrightText, Qt.red)
        dark_palette.setColor(QPalette.Highlight, QColor(42, 130, 218))
        dark_palette.setColor(QPalette.HighlightedText, Qt.black)
        dark_palette.setColor(QPalette.Disabled, QPalette.Text, QColor(128, 128, 128))
        dark_palette.setColor(QPalette.Disabled, QPalette.ButtonText, QColor(128, 128, 128))
        dark_palette.setColor(QPalette.Disabled, QPalette.WindowText, QColor(128, 128, 128))

        QApplication.instance().setPalette(dark_palette)
        self.image_scene.setBackgroundBrush(QBrush(QColor(33, 33, 33)))
        self.update_button_icons()

    def update_button_icons(self):
        self.select_source_btn.setIcon(QIcon(self.get_icon_path("folder")))
        self.select_destination_btn.setIcon(QIcon(self.get_icon_path("folder")))
        self.organize_photos_btn.setIcon(QIcon(self.get_icon_path("organize")))

    def get_icon_path(self, icon_name):
        return resource_path(f"{icon_name}.png")

    def selectSourceDirectory(self):
        directory = QFileDialog.getExistingDirectory(self, "Select Source Directory")
        if directory:
            self.source_directory = directory
            self.source_path_control.setText(directory)
            self.photos_list.clear()

            image_extensions = ('.png', '.jpg', '.jpeg', '.gif', '.bmp', '.tiff', '.webp')

            for root, _, files in os.walk(self.source_directory):
                for file_name in files:
                    if file_name.lower().endswith(image_extensions):
                        file_path = os.path.join(root, file_name)
                        photo = self.extractPhotoMetadata(file_path)
                        if photo:
                            self.photos_list.append(photo)

            self.photos_list.sort(key=lambda p: p.date_time if p.date_time else p.source_path)
            self.populatePhotosTable()

    def selectDestinationDirectory(self):
        directory = QFileDialog.getExistingDirectory(self, "Select Destination Directory")
        if directory:
            self.destination_directory = directory
            self.destination_path_control.setText(directory)

    def onPhotoSelected(self, selected, deselected):
        if selected.indexes():
            index = selected.indexes()[0]
            row = index.row()
            if 0 <= row < len(self.photos_list):
                self.current_selected_photo = self.photos_list[row]
                self.displayPhotoDetails(self.current_selected_photo)
        else:
            self.displayPhotoDetails(None)

    def displayPhotoDetails(self, photo: Photo):
        if not photo:
            self.image_scene.clear()
            self.image_scene.setBackgroundBrush(QBrush(QColor(33, 33, 33)))
            self.updateMetadataLabel(self.width_val, "")
            self.updateMetadataLabel(self.height_val, "")
            self.updateMetadataLabel(self.dpi_val, "")
            self.updateMetadataLabel(self.depth_val, "")
            self.updateMetadataLabel(self.color_val, "")
            return

        try:
            pil_image = Image.open(photo.source_path)

            qimage = ImageQt.ImageQt(pil_image.convert('RGBA'))
            pixmap = QPixmap.fromImage(qimage)

            view_width = self.preview_iv.width()
            view_height = self.preview_iv.height()

            if not pixmap.isNull():
                self.image_scene.clear()
                scaled_pixmap = pixmap.scaled(view_width, view_height, Qt.KeepAspectRatio, Qt.SmoothTransformation)
                item = self.image_scene.addPixmap(scaled_pixmap)

                item_x = (view_width - scaled_pixmap.width()) / 2
                item_y = (view_height - scaled_pixmap.height()) / 2
                item.setPos(item_x, item_y)

                self.preview_iv.fitInView(item, Qt.KeepAspectRatio)
            else:
                self.image_scene.clear()
                error_pixmap = QPixmap(view_width, view_height)
                error_pixmap.fill(QApplication.instance().palette().color(QPalette.AlternateBase))
                painter = QPainter(error_pixmap)
                painter.setPen(QApplication.instance().palette().color(QPalette.Highlight))
                font = painter.font()
                font.setPointSize(14)
                painter.setFont(font)
                text_rect = error_pixmap.rect()
                painter.drawText(text_rect, Qt.AlignCenter, "Error loading image")
                painter.end()
                item = self.image_scene.addPixmap(error_pixmap)
                self.preview_iv.fitInView(item, Qt.KeepAspectRatio)
        except Exception as e:
            self.image_scene.clear()
            view_width = self.preview_iv.width()
            view_height = self.preview_iv.height()
            error_pixmap = QPixmap(view_width, view_height)
            error_pixmap.fill(QApplication.instance().palette().color(QPalette.AlternateBase))
            painter = QPainter(error_pixmap)
            painter.setPen(QApplication.instance().palette().color(QPalette.Highlight))
            font = painter.font()
            font.setPointSize(14)
            painter.setFont(font)
            text_rect = error_pixmap.rect()
            painter.drawText(text_rect, Qt.AlignCenter, f"Error displaying image: {e}")
            painter.end()
            item = self.image_scene.addPixmap(error_pixmap)
            self.preview_iv.fitInView(item, Qt.KeepAspectRatio)

        self.updateMetadataLabel(self.width_val, f"{photo.width} px")
        self.updateMetadataLabel(self.height_val, f"{photo.height} px")
        self.updateMetadataLabel(self.dpi_val, str(photo.dpi))
        self.updateMetadataLabel(self.depth_val, photo.depth)
        self.updateMetadataLabel(self.color_val, photo.color_model)

    def openPreview(self):
        if self.current_selected_photo:
            file_url = QUrl.fromLocalFile(self.current_selected_photo.source_path)
            if not QDesktopServices.openUrl(file_url):
                QMessageBox.warning(self, "Error", f"Could not open file: {self.current_selected_photo.source_path}")
        else:
            QMessageBox.information(self, "No Photo Selected", "Please select a photo from the list to open its preview.")

    def organizePhotos(self):
        if not self.destination_directory:
            QMessageBox.warning(self, "No Destination", "Please select a destination directory first.")
            return
        if not self.photos_list:
            QMessageBox.information(self, "No Photos", "No photos to organize in the list.")
            return

        reply = QMessageBox.question(
            self, 'Clear Destination Folder',
            "Do you want to clear the destination folder before organizing photos? "
            "This will delete all its contents.",
            QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.Yes:
            if os.path.exists(self.destination_directory):
                try:
                    for item in os.listdir(self.destination_directory):
                        item_path = os.path.join(self.destination_directory, item)
                        if os.path.isfile(item_path) or os.path.islink(item_path):
                            os.unlink(item_path)
                        elif os.path.isdir(item_path):
                            shutil.rmtree(item_path)
                    QMessageBox.information(self, "Folder Cleared", "Destination folder cleared successfully.")
                except Exception as e:
                    QMessageBox.critical(self, "Error Clearing Folder", f"Failed to clear destination folder: {e}")
                    return

        structure_format = self.structure_combobox.currentText()
        rename_by_date = self.rename_by_date_swt.isChecked()

        for photo in self.photos_list:
            try:
                dest_path = self.destination_directory
                dt_obj = None

                if photo.date_time:
                    try:
                        dt_obj = datetime.strptime(photo.date_time, "%Y:%m:%d %H:%M:%S")
                    except ValueError:
                        dt_obj = None

                if dt_obj is not None:
                    if structure_format == "yyyy":
                        sub_dir = dt_obj.strftime("%Y")
                    elif structure_format == "yyyy/mm":
                        sub_dir = dt_obj.strftime("%Y/%m")
                    elif structure_format == "yyyy/mm/dd":
                        sub_dir = dt_obj.strftime("%Y/%m/%d")
                    else:
                        sub_dir = ""
                    dest_path = os.path.join(dest_path, sub_dir)

                os.makedirs(dest_path, exist_ok=True)

                original_name = os.path.basename(photo.source_path)
                _, ext = os.path.splitext(original_name)

                if rename_by_date and dt_obj is not None:
                    new_base = dt_obj.strftime("%Y-%m-%d_%H-%M-%S")
                    file_name = f"{new_base}{ext}"
                else:
                    file_name = original_name

                final_dest_file_path = os.path.join(dest_path, file_name)

                counter = 1
                base_no_ext, ext_only = os.path.splitext(final_dest_file_path)
                while os.path.exists(final_dest_file_path):
                    final_dest_file_path = f"{base_no_ext}_{counter}{ext_only}"
                    counter += 1

                shutil.copy2(photo.source_path, final_dest_file_path)

            except Exception as e:
                QMessageBox.critical(self, "Error Organizing", f"Failed to organize {photo.source_path}: {e}")

        QMessageBox.information(self, "Organization Complete", "Photos organized successfully!")

    def showAboutDialog(self):
        about_text = f"""
        <b>{APP_NAME}</b><br>
        Version: {VERSION}<br>
        Author: {AUTHOR}<br><br>
        A simple application to organize your photos
        """
        QMessageBox.about(self, "About", about_text)

    def readLicenseFile(self) -> str:
        license_path = resource_path("license.txt")
        try:
            with open(license_path, "r", encoding="utf-8") as f:
                return f.read()
        except FileNotFoundError:
            return "License file (resources/license.txt) not found."
        except Exception as e:
            return f"Error reading license file: {e}"

    def showLicenseDialog(self):
        license_text = self.readLicenseFile()

        msg_box = QMessageBox(self)
        msg_box.setWindowTitle("License Information")
        msg_box.setTextFormat(Qt.PlainText)
        msg_box.setText(license_text)

        if not self.app_icon.isNull():
            msg_box.setIconPixmap(self.app_icon.pixmap(64, 64))
        msg_box.setStandardButtons(QMessageBox.Ok)
        msg_box.exec()

    def extractPhotoMetadata(self, file_path: str):
        try:
            with Image.open(file_path) as img:
                width = img.width
                height = img.height

                dpi_x, dpi_y = img.info.get('dpi', (0, 0))
                dpi = int(dpi_x) if dpi_x else 0

                depth = img.mode
                if img.mode == '1':
                    depth = "1 bit (B&W)"
                elif img.mode == 'L':
                    depth = "8 bits (Greyscale)"
                elif img.mode == 'P':
                    depth = "8 bits (Indexed)"
                elif img.mode == 'RGB':
                    depth = "24 bits (RGB)"
                elif img.mode == 'RGBA':
                    depth = "32 bits (RGBA)"
                elif img.mode == 'CMYK':
                    depth = "32 bits (CMYK)"
                elif img.mode in ['YCbCr', 'LAB', 'HSV']:
                    depth = f"{len(img.getbands()) * 8} bits ({img.mode})"
                else:
                    depth = f"{len(img.getbands()) * 8} bits (Unknown)"

                color_model = img.mode

                date_time = ""
                exif_data = img._getexif()
                if exif_data:
                    for tag, value in exif_data.items():
                        decoded = ExifTags.TAGS.get(tag, tag)
                        if decoded == 'DateTimeOriginal':
                            date_time = value

                return Photo(source_path=file_path, date_time=date_time,
                             width=width, height=height, dpi=int(dpi),
                             depth=depth,
                             color_model=color_model)
        except Exception as e:
            print(f"Error extracting metadata from {file_path}: {e}")
            return None

    def populatePhotosTable(self):
        self.photos_table_model.setRowCount(0)
        for photo in self.photos_list:
            row_position = self.photos_table_model.rowCount()
            self.photos_table_model.insertRow(row_position)

            file_name_only = os.path.basename(photo.source_path)

            file_name_item = QStandardItem(file_name_only)
            self.photos_table_model.setItem(row_position, 0, file_name_item)

            date_time_str = photo.date_time
            try:
                dt_obj = datetime.strptime(photo.date_time, "%Y:%m:%d %H:%M:%S")
                date_time_str = dt_obj.strftime("%Y/%m/%d - %H:%M:%S")
            except Exception:
                pass
            date_time_item = QStandardItem(date_time_str if date_time_str else "N/A")
            date_time_item.setTextAlignment(Qt.AlignCenter)
            self.photos_table_model.setItem(row_position, 1, date_time_item)


if __name__ == "__main__":
    QApplication.setHighDpiScaleFactorRoundingPolicy(
        Qt.HighDpiScaleFactorRoundingPolicy.PassThrough
    )
    app = QApplication(sys.argv)
    app.setStyle("Fusion")
    window = PhotoOrganizerApp()
    window.show()
    sys.exit(app.exec())