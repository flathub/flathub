# Install Python modules in Spyder

To install more Python modules from pip, use console or spyder-terminal in Spyder, for example:<br />
In spyder console: `!python3 -mpip install thermo`<br />
In spyder-terminal: `python3 -mpip install thermo`<br />

After installing Python modules, the Python modules directory below will be created.<br />
In your file explorer, unhide the hidden folder to see .var folder.<br />
To make sure Python modules installed from pip are found, add<br />
  `/home/USER/.var/app/org.spyder_ide.spyder/data/python/lib/python3.11/`<br />
to PYTHONPATH in Spyder under the tools>PYTHONPATH manager<br />
Then close the console in Spyder to open a new console to take effect.<br />

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
