#! /bin/bash
# This generate dependencies for Spyder
# Mark the script file as executable and run with ./generate_python_deps.sh

# This create some dependencies that is missing
python3 flatpak-pip-generator setuptools_rust hatchling -o spyder_additional_deps &&
# Remove previous text file if any
rm -f spyder_*.txt || true &&
# pipgrip generate list of dependencies of spyder with pip and write it to a text file, install pipgrip with 'pip3 install pipgrip'
pipgrip spyder > spyder_pipgrip.txt &&
# Create a copy and we will work with the copy, pipgrip take a long time
cp spyder_pipgrip.txt spyder_deps.txt &&
# Remove deps that is already installed
sed -i -E '/^(spyder|pyqt|markupsafe|pygments|six)/d' spyder_deps.txt &&
# Move python lib that requires rust to spyder_deps_rust.txt. Rust dependencies is complicate
grep -E '^(jellyfish|jsonschema|rpds|cryptography|referencing|keyring|secretstorage|nbconvert|nbclient|nbformat|python-lsp-black|black)' spyder_deps.txt >> spyder_deps_rust.txt &&
sed -i -E '/^(jellyfish|jsonschema|rpds|cryptography|referencing|keyring|secretstorage|nbconvert|nbclient|nbformat|python-lsp-black|black)/d' spyder_deps.txt &&
# Generate .json file from spyder_deps.txt while ignoring some deps that is already include in the sdk
python3 flatpak-pip-generator --requirements-file spyder_deps.txt --ignore-installed MarkupSafe,pygments,six -o spyder_deps &&
# Generate deps with req2flatpak for precompile lib because build from source need rust deps, install req2flatpak with 'pip3 install req2flatpak'
req2flatpak --requirements-file spyder_deps_rust.txt --target-platforms 310-x86_64 310-aarch64 --outfile spyder_deps_rust.json &&
# Generate recommended deps for some numerical libs for spyder
python3 flatpak-pip-generator pybind11 pyparsing==3.0.9 pillow cppy kiwisolver fonttools cycler meson-python contourpy openpyxl versioneer pandas pythran scipy sympy statsmodels --ignore-installed MarkupSafe,pygments,six -o spyder_deps_numerical &&
# Generate deps for spyder terminal plugins
python3 flatpak-pip-generator terminado tornado coloredlogs -o spyder_deps_terminal &&
# Remove text files
rm -f spyder_*.txt || true &&
# Build the manifest, if not, just comment out
flatpak-builder build --force-clean --install --user *.yaml
