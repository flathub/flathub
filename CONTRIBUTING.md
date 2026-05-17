### Reporting a packaging bug

Please open an issue in this repository if the problem is specific to the Flatpak build
(build failure, missing dependency, wrong permission, etc.).

For bugs in the application itself, open an issue upstream:
https://github.com/D3M-Sudo/Anura/issues

### Updating the manifest

To update a dependency version:
1. Fork this repository
2. Update the relevant `url`, `tag`, `commit`, or `sha256` fields in `com.github.d3msudo.anura.json`
3. Open a pull request against the `master` branch

To update the Anura version itself, update the `anura` module source with the new tag and commit hash.

### Building locally

```bash
flatpak-builder --install --user builddir com.github.d3msudo.anura.json
flatpak run com.github.d3msudo.anura
```
