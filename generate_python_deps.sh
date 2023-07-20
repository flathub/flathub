#! /bin/bash
# This is a bash script that generate dependencies for Spyder
# The generated files are spyder_deps_additional.json spyder_deps.json spyder_deps_numerical.json spyder_deps_rust.json spyder_deps_terminal.json
# Mark the script file as executable and run with ./generate_python_deps.sh


python3 flatpak-pip-generator setuptools_rust hatchling -o spyder_deps_additional && # This create some dependencies that is missing
rm -f spyder_*.txt || true && # Remove previous text file if any
pipgrip spyder > spyder_pipgrip.txt && # pipgrip generate list of dependencies of spyder with pip and write it to a text file, install pipgrip with 'pip3 install pipgrip'
cp spyder_pipgrip.txt spyder_deps_1.txt && # Create a copy and we will work with the copy, pipgrip take a long time
sed -i -E '/^(spyder|pyqt|markupsafe|pygments|six)/d' spyder_deps_1.txt && # Remove deps that is already installed
# Move python lib that requires rust to spyder_deps_rust.txt. Rust dependencies is complicated
grep -E '^(jellyfish|jsonschema|rpds|cryptography|referencing|keyring|secretstorage|nbconvert|nbclient|nbformat|python-lsp-black|black)' spyder_deps_1.txt >> spyder_deps_rust.txt &&
sed -i -E '/^(jellyfish|jsonschema|rpds|cryptography|referencing|keyring|secretstorage|nbconvert|nbclient|nbformat|python-lsp-black|black)/d' spyder_deps_1.txt &&
# The spyder_deps_1.txt will generate too large of a json file so split them to spyder_deps_2.txt
sed -n '1,60p' spyder_deps_1.txt > spyder_deps_1_temp.txt  # Save the first 10 lines of fileA to a temporary file
sed -n '61,$p' spyder_deps_1.txt > spyder_deps_2.txt      # Save lines 11 and beyond to fileB
mv spyder_deps_1_temp.txt spyder_deps_1.txt               # Overwrite fileA with the first 10 lines from the temporary file
# Generate .json file from spyder_deps_1.txt while ignoring some deps that is already include in the sdk
python3 flatpak-pip-generator --requirements-file spyder_deps_1.txt --ignore-installed MarkupSafe,pygments,six -o spyder_deps_1 &&
python3 flatpak-pip-generator --requirements-file spyder_deps_2.txt --ignore-installed MarkupSafe,pygments,six -o spyder_deps_2 &&
# Generate deps with req2flatpak for precompile lib because build from source need rust deps, install req2flatpak with 'pip3 install req2flatpak'
req2flatpak --requirements-file spyder_deps_rust.txt --target-platforms 310-x86_64 310-aarch64 --outfile spyder_deps_rust.json &&
# Generate recommended deps for some numerical libs for spyder, Matplotlib have issue building with newer pyparsing
python3 flatpak-pip-generator pybind11 pyparsing==3.0.9 pillow cppy kiwisolver fonttools cycler meson-python contourpy openpyxl versioneer pandas pythran scipy sympy statsmodels --ignore-installed MarkupSafe,pygments,six -o spyder_deps_numerical &&
python3 flatpak-pip-generator terminado tornado coloredlogs -o spyder_deps_terminal && # Generate deps for spyder terminal plugins
rm -f spyder_*.txt || true && # Remove text files
flatpak-builder build --force-clean --install --user *.yaml # Build the manifest, if not, just comment out
