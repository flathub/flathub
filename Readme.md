# Updating dependencies:
1) Download Flatpak PIP Generator
https://github.com/flatpak/flatpak-builder-tools/tree/master/pip
https://docs.flatpak.org/en/latest/python.html


2) Execute Flatpak PIP Generator (Add dependencies if they are needed)
python flatpak-pip-generator PyQt5-sip cppy setuptools numpy cycler python-dateutil pyparsing fonttools kiwisolver pyparsing matplotlib docutils xlwt scipy

3) Replace python-modules.json file in this repository
