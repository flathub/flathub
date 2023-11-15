# Install Python modules in Spyder
Use console in Spyder to install more Python modules.
For example `!python3 -mpip install thermo`.
`/.var/app/org.spyder_ide.spyder/data/python/lib/python3.11`
# Update Python modules
The executable generate_python_deps.sh generate dependencies for Spyder.<br />
Use `./generate_python_deps.sh` to run that script to update dependencies.<br />
Extra dependencies are generated with flatpak-pip-generator<br />
pipgrip create dependencies from Spyder called spyder_deps.txt.<br />
Un-use dependencies like PyQt is removed.<br />
Dependencies that require rust are moved to spyder_deps_rust.txt.<br />
flatpak-pip-generator generate json dependencies from spyder_deps.txt<br />
req2flatpak generate json dependencies from spyder_deps_rust.txt which is pre-compiled<br />
flatpak-pip-generator generate recommended package by Spyder<br />
flatpak-pip-generator generate dependencies needed by spyder-terminal plugin<br />
