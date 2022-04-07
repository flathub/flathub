# Creating flatpak package manually
See build.sh



# Updating dependencies:
1) Download Flatpak PIP Generator
https://github.com/flatpak/flatpak-builder-tools/tree/master/pip
https://docs.flatpak.org/en/latest/python.html


2) Execute Flatpak PIP Generator (Add dependencies if they are needed)
`python flatpak-pip-generator PyQt5-sip matplotlib docutils xlwt scipy`

3) Replace python-modules.json file in this repository


# Throubleshooting
if the build fails check out on similar projects which are using python
https://github.com/flathub/org.paraview.ParaView

PyQt5:
https://github.com/flathub/org.plomgrading.PlomClient
