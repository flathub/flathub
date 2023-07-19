generate_python_deps.sh generate dependencies for Spyder.

Use ./generate_python_deps.sh to run that script to update dependencies.

Extra dependencies are generated (setuptools_rust and hatchling) with flatpak-pip-generator
pipgrip create dependencies from Spyder called spyder_deps.txt.
Un-use dependencies like PyQt is removed.
Dependencies that require rust are moved to spyder_deps_rust.txt.
flatpak-pip-generator generate json dependencies from spyder_deps.txt
req2flatpak generate json dependencies from spyder_deps_rust.txt which is pre-compiled
flatpak-pip-generator generate recommended package by Spyder
flatpak-pip-generator generate dependencies needed by spyder-terminal plugin
