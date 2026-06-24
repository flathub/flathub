# Flatpak Builder Tools

This repository contains a collection of various scripts to aid in using
`flatpak-builder`.

Feel free to submit your own scripts that would be useful for others.

The intended usage of the generators is as a submodule used as part of your build
process to generate manifests.

See the sub-directories of the respective tools for more information and licenses.

## Flutter

Please look at the [flatpak-flutter](https://github.com/TheAppgineer/flatpak-flutter)
project, developed by [Jan Koudijs](https://github.com/JanKoudijs).

### Converting manifests from JSON to YAML

A simple script to help convert JSON manifests to YAML is also in this repo.

After cloning the repository you can run `./flatpak-json2yaml.py /path/to/example.json --output=example.yml`.

This depends on [PyYAML](https://pypi.org/project/PyYAML/) which may need to be installed.
