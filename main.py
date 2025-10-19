#!/usr/bin/env python3
"""
Zippy - Professional Folder Compression Tool
A powerful GUI application for compressing large folders efficiently
Requirements: pip install PyQt6
"""

import sys
import os
import zipfile
from pathlib import Path
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                             QHBoxLayout, QPushButton, QLabel, QLineEdit, 
                             QFileDialog, QProgressBar, QCheckBox, QTextEdit,
                             QGroupBox, QMessageBox, QDialog)
from PyQt6.QtCore import QThread, pyqtSignal, Qt
from PyQt6.QtGui import QFont, QIcon, QPixmap, QPainter, QColor


class ZipWorker(QThread):
    progress = pyqtSignal(int, str)
    finished = pyqtSignal(bool, str)
    
    def __init__(self, source_dir, output_file, exclude_patterns):
        super().__init__()
        self.source_dir = source_dir
        self.output_file = output_file
        self.exclude_patterns = exclude_patterns
        self.is_cancelled = False
        
    def cancel(self):
        self.is_cancelled = True
        
    def should_exclude(self, path):
        """Check if file or folder should be excluded"""
        path_str = str(path)
        for pattern in self.exclude_patterns:
            if pattern in path_str:
                return True
        return False
        
    def run(self):
        try:
            source_path = Path(self.source_dir)
            all_files = []
            
            # Collect all files first
            self.progress.emit(0, "Scanning files...")
            for root, dirs, files in os.walk(self.source_dir):
                # Skip excluded directories
                dirs[:] = [d for d in dirs if not self.should_exclude(Path(root) / d)]
                
                for file in files:
                    file_path = Path(root) / file
                    if not self.should_exclude(file_path):
                        all_files.append(file_path)
            
            total_files = len(all_files)
            if total_files == 0:
                self.finished.emit(False, "No files found to compress!")
                return
                
            # Create zip file
            self.progress.emit(0, f"Found {total_files} files, creating archive...")
            
            with zipfile.ZipFile(self.output_file, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as zipf:
                for idx, file_path in enumerate(all_files):
                    if self.is_cancelled:
                        self.finished.emit(False, "Operation cancelled!")
                        return
                        
                    try:
                        arcname = file_path.relative_to(source_path)
                        zipf.write(file_path, arcname)
                        
                        progress_pct = int((idx + 1) / total_files * 100)
                        self.progress.emit(progress_pct, f"Processing: {arcname}")
                    except Exception as e:
                        print(f"Error (skipped): {file_path} - {e}")
            
            # Format file size
            size_bytes = os.path.getsize(self.output_file)
            if size_bytes >= 1024**3:  # GB
                size_str = f"{size_bytes / (1024**3):.2f} GB"
            elif size_bytes >= 1024**2:  # MB
                size_str = f"{size_bytes / (1024**2):.2f} MB"
            elif size_bytes >= 1024:  # KB
                size_str = f"{size_bytes / 1024:.2f} KB"
            else:
                size_str = f"{size_bytes} Bytes"
            
            self.finished.emit(True, f"Successfully completed!\nOutput size: {size_str}")
            
        except Exception as e:
            self.finished.emit(False, f"Error occurred: {str(e)}")


class AboutDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("About Zippy")
        self.setFixedSize(450, 350)
        self.init_ui()
        
    def init_ui(self):
        layout = QVBoxLayout(self)
        
        # Icon
        icon_label = QLabel()
        icon_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        icon_pixmap = self.load_logo(64)
        icon_label.setPixmap(icon_pixmap)
        layout.addWidget(icon_label)
        
        # App name
        name_label = QLabel("Zippy")
        name_font = QFont()
        name_font.setPointSize(24)
        name_font.setBold(True)
        name_label.setFont(name_font)
        name_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(name_label)
        
        # Version
        version_label = QLabel("Version 1.0.0")
        version_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        version_label.setStyleSheet("color: #666;")
        layout.addWidget(version_label)
        
        # Description
        desc_label = QLabel("Professional Folder Compression Tool")
        desc_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        desc_label.setStyleSheet("margin: 10px 0px;")
        layout.addWidget(desc_label)
        
        # Developer info
        info_widget = QWidget()
        info_layout = QVBoxLayout(info_widget)
        info_layout.setSpacing(5)
        
        developer = QLabel("<b>Developer:</b> Fatih KURT")
        company = QLabel("<b>Company:</b> <a href='https://fktyazilim.com'>FKT Yazılım</a>")
        company.setOpenExternalLinks(True)
        email = QLabel("<b>Email:</b> <a href='mailto:info@fktyazilim.com'>info@fktyazilim.com</a>")
        email.setOpenExternalLinks(True)
        
        info_layout.addWidget(developer)
        info_layout.addWidget(company)
        info_layout.addWidget(email)
        
        info_widget.setStyleSheet("""
            QLabel { padding: 3px; }
            QLabel a { color: #2563eb; text-decoration: none; }
            QLabel a:hover { text-decoration: underline; }
        """)
        
        layout.addWidget(info_widget)
        layout.addStretch()
        
        # Close button
        close_btn = QPushButton("Close")
        close_btn.clicked.connect(self.close)
        layout.addWidget(close_btn)
    
    def load_logo(self, size):
        """Load PNG logo and resize it"""
        try:
            logo_path = "images/logo.png"
            if os.path.exists(logo_path):
                pixmap = QPixmap(logo_path)
                if not pixmap.isNull():
                    # Scale to desired size while maintaining aspect ratio
                    scaled_pixmap = pixmap.scaled(size, size, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation)
                    return scaled_pixmap
        except Exception as e:
            print(f"PNG loading error: {e}")
        
        # Fallback to created icon if PNG fails
        return self.create_app_icon(size)
    
    def create_app_icon(self, size):
        """Create app icon as fallback"""
        pixmap = QPixmap(size, size)
        pixmap.fill(Qt.GlobalColor.transparent)
        
        painter = QPainter(pixmap)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        # Gradient background
        painter.setBrush(QColor("#3b82f6"))
        painter.setPen(Qt.PenStyle.NoPen)
        painter.drawRoundedRect(0, 0, size, size, size//8, size//8)
        
        # ZIP text
        painter.setPen(QColor("white"))
        font = QFont()
        font.setPointSize(size//4)
        font.setBold(True)
        painter.setFont(font)
        painter.drawText(pixmap.rect(), Qt.AlignmentFlag.AlignCenter, "Z")
        
        painter.end()
        return pixmap


class Zippy(QMainWindow):
    def __init__(self):
        super().__init__()
        self.worker = None
        self.init_ui()
        
    def init_ui(self):
        self.setWindowTitle("Zippy - Professional Folder Compression")
        self.setMinimumSize(900, 700)
        
        # Set window icon
        icon_pixmap = self.load_logo(32)
        self.setWindowIcon(QIcon(icon_pixmap))
        
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QVBoxLayout(central_widget)
        layout.setSpacing(15)
        
        # Header with icon and title
        header_layout = QHBoxLayout()
        icon_label = QLabel()
        icon_label.setPixmap(self.load_logo(48))
        header_layout.addWidget(icon_label)
        
        title = QLabel("Zippy")
        title_font = QFont()
        title_font.setPointSize(22)
        title_font.setBold(True)
        title.setFont(title_font)
        header_layout.addWidget(title)
        
        header_layout.addStretch()
        
        # About button
        about_btn = QPushButton("About")
        about_btn.clicked.connect(self.show_about)
        header_layout.addWidget(about_btn)
        
        layout.addLayout(header_layout)
        
        subtitle = QLabel("Compress large folders efficiently with smart exclusions")
        subtitle.setStyleSheet("color: #666; font-size: 12px;")
        layout.addWidget(subtitle)
        
        # Source folder selection
        source_group = QGroupBox("Source Folder")
        source_layout = QHBoxLayout()
        self.source_input = QLineEdit()
        self.source_input.setPlaceholderText("Select folder to compress...")
        self.source_btn = QPushButton("Browse")
        self.source_btn.clicked.connect(self.select_source)
        source_layout.addWidget(self.source_input)
        source_layout.addWidget(self.source_btn)
        source_group.setLayout(source_layout)
        layout.addWidget(source_group)
        
        # Output file selection
        output_group = QGroupBox("Output Location")
        output_layout = QHBoxLayout()
        self.output_input = QLineEdit()
        self.output_input.setPlaceholderText("Where to save the zip file...")
        self.output_btn = QPushButton("Browse")
        self.output_btn.clicked.connect(self.select_output)
        output_layout.addWidget(self.output_input)
        output_layout.addWidget(self.output_btn)
        output_group.setLayout(output_layout)
        layout.addWidget(output_group)
        
        # Exclusion options
        exclude_group = QGroupBox("Smart Exclusions - Skip These Folders")
        exclude_layout = QVBoxLayout()
        
        # Common development folders
        dev_label = QLabel("Development Files:")
        dev_font = QFont()
        dev_font.setBold(True)
        dev_font.setPointSize(10)
        dev_label.setFont(dev_font)
        exclude_layout.addWidget(dev_label)
        
        self.exclude_node_modules = QCheckBox("node_modules (Node.js/npm)")
        self.exclude_node_modules.setChecked(True)
        self.exclude_vendor = QCheckBox("vendor (PHP/Composer)")
        self.exclude_vendor.setChecked(True)
        self.exclude_venv = QCheckBox("venv / env (Python virtual environments)")
        self.exclude_venv.setChecked(True)
        self.exclude_target = QCheckBox("target (Rust/Cargo)")
        self.exclude_target.setChecked(True)
        
        exclude_layout.addWidget(self.exclude_node_modules)
        exclude_layout.addWidget(self.exclude_vendor)
        exclude_layout.addWidget(self.exclude_venv)
        exclude_layout.addWidget(self.exclude_target)
        
        # Build & cache folders
        build_label = QLabel("<b>Build & Cache Files:</b>")
        build_label.setStyleSheet("margin-top: 10px;")
        exclude_layout.addWidget(build_label)
        
        self.exclude_dist = QCheckBox("dist / build / out (Build outputs)")
        self.exclude_dist.setChecked(True)
        self.exclude_next = QCheckBox(".next (Next.js)")
        self.exclude_next.setChecked(True)
        self.exclude_cache = QCheckBox("__pycache__ / .cache (Cache files)")
        self.exclude_cache.setChecked(True)
        
        exclude_layout.addWidget(self.exclude_dist)
        exclude_layout.addWidget(self.exclude_next)
        exclude_layout.addWidget(self.exclude_cache)
        
        # Version control
        vc_label = QLabel("<b>Version Control:</b>")
        vc_label.setStyleSheet("margin-top: 10px;")
        exclude_layout.addWidget(vc_label)
        
        self.exclude_git = QCheckBox(".git (Git repository)")
        self.exclude_git.setChecked(True)
        self.exclude_svn = QCheckBox(".svn (Subversion)")
        self.exclude_svn.setChecked(True)
        
        exclude_layout.addWidget(self.exclude_git)
        exclude_layout.addWidget(self.exclude_svn)
        
        # IDE folders
        ide_label = QLabel("<b>IDE Files:</b>")
        ide_label.setStyleSheet("margin-top: 10px;")
        exclude_layout.addWidget(ide_label)
        
        self.exclude_idea = QCheckBox(".idea / .vscode (IDE settings)")
        self.exclude_idea.setChecked(True)
        
        exclude_layout.addWidget(self.exclude_idea)
        
        exclude_group.setLayout(exclude_layout)
        layout.addWidget(exclude_group)
        
        # Progress bar
        self.progress_bar = QProgressBar()
        self.progress_bar.setVisible(False)
        layout.addWidget(self.progress_bar)
        
        # Status log
        self.log_text = QTextEdit()
        self.log_text.setReadOnly(True)
        self.log_text.setMaximumHeight(150)
        layout.addWidget(self.log_text)
        
        # Action buttons
        button_layout = QHBoxLayout()
        self.start_btn = QPushButton("🚀 Start Compression")
        self.start_btn.clicked.connect(self.start_zipping)
        self.start_btn.setMinimumHeight(45)
        self.start_btn.setStyleSheet("""
            QPushButton {
                background-color: #3b82f6;
                color: white;
                font-weight: bold;
                border-radius: 5px;
            }
            QPushButton:hover {
                background-color: #2563eb;
            }
        """)
        
        self.cancel_btn = QPushButton("❌ Cancel")
        self.cancel_btn.clicked.connect(self.cancel_zipping)
        self.cancel_btn.setEnabled(False)
        self.cancel_btn.setMinimumHeight(45)
        self.cancel_btn.setStyleSheet("""
            QPushButton {
                background-color: #ef4444;
                color: white;
                font-weight: bold;
                border-radius: 5px;
            }
            QPushButton:hover {
                background-color: #dc2626;
            }
        """)
        
        button_layout.addWidget(self.start_btn)
        button_layout.addWidget(self.cancel_btn)
        layout.addLayout(button_layout)
        
        # Info label
        info_label = QLabel("💡 Tip: Smart exclusions reduce file size by skipping dependencies and temporary files")
        info_label.setWordWrap(True)
        info_label.setStyleSheet("color: #666; font-style: italic; padding: 10px;")
        layout.addWidget(info_label)
    
    def load_logo(self, size):
        """Load PNG logo and resize it"""
        try:
            logo_path = "images/logo.png"
            if os.path.exists(logo_path):
                pixmap = QPixmap(logo_path)
                if not pixmap.isNull():
                    # Scale to desired size while maintaining aspect ratio
                    scaled_pixmap = pixmap.scaled(size, size, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation)
                    return scaled_pixmap
        except Exception as e:
            print(f"PNG loading error: {e}")
        
        # Fallback to created icon if PNG fails
        return self.create_app_icon(size)
    
    def create_app_icon(self, size):
        """Create app icon as fallback"""
        pixmap = QPixmap(size, size)
        pixmap.fill(Qt.GlobalColor.transparent)
        
        painter = QPainter(pixmap)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        # Gradient background
        painter.setBrush(QColor("#3b82f6"))
        painter.setPen(Qt.PenStyle.NoPen)
        painter.drawRoundedRect(0, 0, size, size, size//8, size//8)
        
        # ZIP text
        painter.setPen(QColor("white"))
        font = QFont()
        font.setPointSize(size//3)
        font.setBold(True)
        painter.setFont(font)
        painter.drawText(pixmap.rect(), Qt.AlignmentFlag.AlignCenter, "Z")
        
        painter.end()
        return pixmap
    
    def show_about(self):
        """Show about dialog"""
        dialog = AboutDialog(self)
        dialog.exec()
        
    def select_source(self):
        folder = QFileDialog.getExistingDirectory(self, "Select Source Folder")
        if folder:
            self.source_input.setText(folder)
            # Auto-suggest output file
            suggested_output = f"{folder}.zip"
            self.output_input.setText(suggested_output)
            
    def select_output(self):
        file_path, _ = QFileDialog.getSaveFileName(
            self, "Save Zip File", "", "Zip File (*.zip)"
        )
        if file_path:
            if not file_path.endswith('.zip'):
                file_path += '.zip'
            self.output_input.setText(file_path)
            
    def get_exclude_patterns(self):
        patterns = []
        if self.exclude_node_modules.isChecked():
            patterns.append('node_modules')
        if self.exclude_vendor.isChecked():
            patterns.append('vendor')
        if self.exclude_venv.isChecked():
            patterns.extend(['venv', 'env', '.env'])
        if self.exclude_target.isChecked():
            patterns.append('target')
        if self.exclude_git.isChecked():
            patterns.append('.git')
        if self.exclude_svn.isChecked():
            patterns.append('.svn')
        if self.exclude_next.isChecked():
            patterns.append('.next')
        if self.exclude_dist.isChecked():
            patterns.extend(['dist', 'build', 'out'])
        if self.exclude_cache.isChecked():
            patterns.extend(['__pycache__', '.cache'])
        if self.exclude_idea.isChecked():
            patterns.extend(['.idea', '.vscode'])
        return patterns
        
    def start_zipping(self):
        source = self.source_input.text()
        output = self.output_input.text()
        
        if not source or not os.path.isdir(source):
            QMessageBox.warning(self, "Error", "Please select a valid source folder!")
            return
            
        if not output:
            QMessageBox.warning(self, "Error", "Please specify output file location!")
            return
            
        # Update UI
        self.start_btn.setEnabled(False)
        self.cancel_btn.setEnabled(True)
        self.progress_bar.setVisible(True)
        self.progress_bar.setValue(0)
        self.log_text.clear()
        self.log_text.append("Starting compression...")
        
        # Start worker thread
        exclude_patterns = self.get_exclude_patterns()
        self.worker = ZipWorker(source, output, exclude_patterns)
        self.worker.progress.connect(self.update_progress)
        self.worker.finished.connect(self.zipping_finished)
        self.worker.start()
        
    def cancel_zipping(self):
        if self.worker:
            self.log_text.append("\n⚠️ Cancelling operation...")
            self.worker.cancel()
            
    def update_progress(self, value, message):
        self.progress_bar.setValue(value)
        self.log_text.append(message)
        self.log_text.verticalScrollBar().setValue(
            self.log_text.verticalScrollBar().maximum()
        )
        
    def zipping_finished(self, success, message):
        self.start_btn.setEnabled(True)
        self.cancel_btn.setEnabled(False)
        
        if success:
            self.log_text.append(f"\n✅ {message}")
            QMessageBox.information(self, "Success", message)
        else:
            self.log_text.append(f"\n❌ {message}")
            QMessageBox.warning(self, "Error", message)


def main():
    app = QApplication(sys.argv)
    app.setStyle('Fusion')  # Modern look
    
    # Set application metadata
    app.setApplicationName("Zippy")
    app.setOrganizationName("FKT Yazılım")
    app.setOrganizationDomain("https:fktyazilim.com")
    
    window = Zippy()
    window.show()
    sys.exit(app.exec())


if __name__ == '__main__':
    main()
