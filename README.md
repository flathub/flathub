Sandglass: A QML GUI Countdown Timer with a Visual Sandglass
Sandglass is a simple, cross-platform GUI application built using Qt and QML. It provides an intuitive interface to set a countdown duration and displays the remaining time via a dynamic, visually appealing sandglass (hourglass) graphic.
It's perfect for timing short tasks, giving a clear graphical cue during presentations, or simply adding a polished timer to your desktop environment.
Features
* Customizable Countdown: Specify the countdown duration easily via an input field or control.
* Dynamic Graphical Sandglass: Displays a smooth, constantly draining sandglass animation built with QML that visually represents the passing time.
* Intuitive Interface: Designed for simple interaction within a desktop environment.
* Cross-Platform: Built on Qt, making it easily deployable across Windows, macOS, and Linux.
* Minimal Dependencies: Designed to be lightweight and easy to run.
Installation
To run Sandglass, you will need the Qt Framework installed, which provides the necessary QML runtime environment.
Prerequisites
You will need the Qt Framework (Qt 5 or Qt 6) runtime and development modules required for QML applications installed on your system.
Option 1: Using Source Code
1. Clone the Repository:
git clone [https://gitlab.com/poltpolt/sandglass.git](https://gitlab.com/poltpolt/sandglass.git)
cd sandglass

2. Run with QML Runtime (Fastest way to test):
If you have the qmlscene utility installed (often part of Qt development tools), you can run the main QML file directly:
qmlscene main.qml

3. Build an Executable (Recommended for Distribution):
For a standalone application, you will need to set up a Qt development environment (like Qt Creator) and compile the project into a native binary.
Option 2: Using a Pre-built Package (Future)
If Sandglass is packaged for your operating system (e.g., via AppImage, Snap, MSI installer, etc.), you can install it easily:
# Example for Linux:
snap install sandglass

Usage
Once the application is launched, you will see the Sandglass window:
   1. Set Duration: Use the input field or controls in the GUI to set the desired countdown time in seconds.
   2. Start Timer: Click the "Start" button (or equivalent control).
   3. Visual Countdown: Watch the sandglass graphic empty as the time counts down.
Contributing
We welcome contributions! As an open-source project licensed under the GPL-3.0, we encourage you to fork the repository, make improvements, and submit merge requests.
   1. Fork the repository.
   2. Create a new feature branch (git checkout -b feature/AmazingFeature).
   3. Commit your changes (git commit -m 'Add some AmazingFeature').
   4. Push to the branch (git push origin feature/AmazingFeature).
   5. Open a Merge Request against the main branch.
As this is a QML/Qt project, please ensure your changes adhere to Qt/C++ standards and include necessary documentation and tests.
License
This project is licensed under the GNU General Public License v3.0 (GPL-3.0).
You are free to use, modify, and distribute this software under the terms of the license. See the LICENSE file for the full text.
Project Maintainer
poltpolt - https://gitlab.com/poltpolt