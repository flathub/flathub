# Regenerating generated-sources.yaml

In rare cases where upstream introduces new zip files, we need to regenerate `generated-sources.yaml` manually.

To make that job easier, [this helper Python script](https://github.com/guihkx/irpf-tools-flatpak/tree/master/sources-generator) can be used:

```sh
# Download the Python script (geneate.py) and its dependencies file (requirements.txt)
echo -n generate.py requirements.txt | xargs -d ' ' -I% curl -LO https://github.com/guihkx/irpf-tools-flatpak/raw/refs/heads/master/sources-generator/%
# Create a virtual Python environment in the '.venv' directory
python3 -m venv .venv
# Activate the environment
source .venv/bin/activate
# Install the script dependencies
pip3 install -Ur requirements.txt
# Regenerate the sources file
python3 generate.py -e 2025 > generated-sources.yaml
```

Then, test the changes by rebuilding and running the app locally.
