### How To Update NitrokeyApp2 Flatpak/Flathub Builder

* copy `poetry.lock` from origin repository
* adapt `Makefile`
    * update `pre-requirements.txt` target versions
    * update `rust-requirements.txt` target versions
* if needed:
    * update `python3-hidapi.json` 
    * update `python3-poetry.json`
* run `make` to create all needed files
* run `make lint` to check for linting issues
* run `make pkg` to test the build


### TODO for next release

* properly set version in `.appdata` inside origin repo
* use correct location for screenshot

