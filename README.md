# `re.fossplant.pbtk`

This repository contains a Flatpak template for building the https://github.com/marin-m/pbtk GUI

## Local build instructions

## Build instructions

Build dependencies:

```
sudo apt install flatpak-builder flatpak build-essential \
    python3-dev python3-pip appstream git
sudo snap install --classic astral-uv
```

Then, run:

```
./build.sh
```

## Utilities

These commands will generate a `python3-pbtk.json` file that can serve as a template for specifying the dependencies into `re.fossplant.pbtk.json`:

```bash
cd
git clone git@github.com:flatpak/flatpak-builder-tools.git
git clone git@github.com:marin-m/re.fossplant.pbtk.git

cd flatpak-builder-tools/pip
uv sync --all-groups --frozen
source .venv/bin/activate

cd ~/re.fossplant.pbtk
~/flatpak-builder-tools/pip/flatpak-pip-generator pbtk
```

Note: PySide6 will be provided by the `io.qt.PySide.BaseApp` Flatpak base, hence it is not in the Python modules list.
