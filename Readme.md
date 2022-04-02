# Creating flatpak package manually
`flatpak-builder --force-clean --repo=pyfdaRepo build com.github.chipmuenk.pyfda.yaml`



# Updating dependencies:
1) Download Flatpak PIP Generator
https://github.com/flatpak/flatpak-builder-tools/tree/master/pip
https://docs.flatpak.org/en/latest/python.html


2) Execute Flatpak PIP Generator (Add dependencies if they are needed)
`python flatpak-pip-generator PyQt5-sip matplotlib docutils xlwt scipy`

3) Replace python-modules.json file in this repository
