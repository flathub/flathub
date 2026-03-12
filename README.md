# Ermis - Steganography Tool

![Ermis Logo](io.github.alamahant.Ermis.png)

Ermis is a cross-platform steganography application that allows you to hide and extract secret data within digital media files. Named after the Greek god of messages and communication, Ermis enables covert communication through innocent-looking image and audio files.

## Features

### 🖼️ Image Steganography
- Hide data in PNG, JPG, and BMP images using LSB (Least Significant Bit) techniques
- Live preview of original and modified images
- Visual comparison between carrier and stego images

### 🎵 Audio Steganography
- Support for WAV, MP3, FLAC, and OGG audio files
- Automatic conversion of non-WAV files to WAV format using FFmpeg
- Preserves audio quality while hiding data

### 📝 Data Handling
- **Text Input**: Hide plain text messages
- **File Input**: Hide any file type (documents, images, archives, etc.)
- **Filename Preservation**: Original filenames are stored and recovered during extraction
- **Smart Detection**: Automatically distinguishes between text and file data

### 🔒 Security Features
- **AES Encryption**: Optional encryption with passphrase protection
- **Passphrase Memory**: Remember passphrases during the current session
- **ENCR Marker**: Automatically detects and handles encrypted data

### 📋 User Interface
- **Dual-Tab Interface**: Separate tabs for hiding and extracting data
- **Drag & Drop**: Simply drag files directly into the application
- **Live Capacity Indicator**: Shows available space in real-time
- **Clipboard Integration**: Copy extracted text with one click
- **Status Bar**: Real-time feedback on operations
- **Directory Fallbacks**: Smart path handling (Pictures → Images, Music → AppDir)

### 🔍 Extraction Features
- Automatic detection of encryption and PRT mode
- Filename recovery from hidden files
- Smart truncation for very large text (prevents UI freezing)
- Save extracted data to any location
- Text preview for readable content

## Screenshots

*[Screenshots would go here]*

## Installation

### Prerequisites
- Qt 6.2 or higher
- FFmpeg libraries
- C++17 compatible compiler

### Building from Source

```bash
# Clone the repository
git clone https://github.com/alamahant/Ermis.git
cd Ermis

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make

# Run
./Ermis
