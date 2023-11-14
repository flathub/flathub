#! /bin/bash
# This is a bash script that generate dependencies for Spyder
# The generated files are spyder_deps_additional.json spyder_deps.json spyder_deps_numerical.json spyder_deps_rust.json spyder_deps_terminal.json
# Mark the script file as executable and run with ./generate_python_deps.sh


python3 flatpak-pip-generator setuptools_rust hatchling exceptiongroup pyproject_metadata tomli setuptools_scm_git_archive setuptools_scm -o spyder_deps_additional && # This create some dependencies that is missing. exceptiongroup needed by ipython 8.15.0
rm -f spyder_*.txt || true && # Remove previous text file if any
pipgrip spyder > spyder_pipgrip.txt && # pipgrip generate list of dependencies of spyder with pip and write it to a text file, install pipgrip with 'pip3 install pipgrip'
cp spyder_pipgrip.txt spyder_deps_list.txt && # Create a copy and we will work with the copy, pipgrip take a long time
sed -i -E '/^(spyder|pyqt|markupsafe|pygments|six)/d' spyder_deps_list.txt && # Remove deps that is already installed
# Move python lib that requires rust to spyder_deps_rust.txt. Rust dependencies is complicated
grep -E '^(jellyfish|jsonschema|rpds|cryptography|referencing|keyring|secretstorage|nbconvert|nbclient|nbformat|python-lsp-black|black)' spyder_deps_list.txt >> spyder_deps_rust.txt &&
sed -i -E '/^(jellyfish|jsonschema|rpds|cryptography|referencing|keyring|secretstorage|nbconvert|nbclient|nbformat|python-lsp-black|black)/d' spyder_deps_list.txt &&
# The spyder_deps_list.txt will generate too large of a json file so split them to spyder_deps_2.txt
sed -n '1,50p' spyder_deps_list.txt > spyder_deps_1.txt  # Save the first 50 lines to spyder_deps_1.txt
sed -n '51,100p' spyder_deps_list.txt > spyder_deps_2.txt
sed -n '101,$p' spyder_deps_list.txt > spyder_deps_3.txt
Generate .json file from spyder_deps_list.txt while ignoring some deps that is already include in the sdk
python3 flatpak-pip-generator --requirements-file spyder_deps_1.txt --ignore-installed MarkupSafe,pygments,six -o spyder_deps_1 &&
python3 flatpak-pip-generator --requirements-file spyder_deps_2.txt --ignore-installed MarkupSafe,pygments,six -o spyder_deps_2 &&
python3 flatpak-pip-generator --requirements-file spyder_deps_3.txt --ignore-installed MarkupSafe,pygments,six -o spyder_deps_3 &&
Generate deps with req2flatpak for precompile lib because build from source need rust deps, install req2flatpak with 'pip3 install req2flatpak'
req2flatpak --requirements-file spyder_deps_rust.txt --target-platforms 311-x86_64 311-aarch64 --outfile spyder_deps_rust.json &&
# Generate recommended deps for some numerical libs for spyder, Matplotlib have issue building with newer pyparsing
python3 flatpak-pip-generator pybind11 pyparsing pillow cppy kiwisolver fonttools cycler meson-python contourpy openpyxl versioneer pandas pythran scipy sympy statsmodels --ignore-installed MarkupSafe,pygments,six -o spyder_deps_numerical &&
python3 flatpak-pip-generator terminado tornado coloredlogs -o spyder_deps_terminal && # Generate deps for spyder terminal plugins
rm -f spyder_*.txt || true # Remove text files
flatpak-builder build --force-clean --install --user org.spyder_ide.spyder.yaml # Build the manifest, if not, just comment out
