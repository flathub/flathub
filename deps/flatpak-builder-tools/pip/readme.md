# Flatpak PIP Generator

Tool to automatically generate `flatpak-builder` manifest json from a `pip`
package-name.

This requires `requirements-parser` which can be installed on your host with `pip3 install --user requirements-parser`.

## Usage

`flatpak-pip-generator --runtime='org.freedesktop.Sdk//22.08' foo` which generates `python3-foo.json` and can be included in a manifest like:

```json
"modules": [
  "python3-foo.json",
  {
    "name": "other-modules"
  }
]
```

You can also list multiple packages in single command:
```
flatpak-pip-generator --runtime='org.freedesktop.Sdk//22.08' foo\>=1.0.0,\<2.0.0 bar 
```

If your project contains a [requirements.txt file](https://pip.readthedocs.io/en/stable/user_guide/#requirements-files) with all the project dependencies, you can use 
```
flatpak-pip-generator --runtime='org.freedesktop.Sdk//22.08' --requirements-file='/the/path/to/requirements.txt' --output pypi-dependencies
```

You can use that in your manifest like 
```json
"modules": [
  "pypi-dependencies.json",
  {
    "name": "other-modules"
  }
]
```

## Options

* `--python2`: Build with Python 2. Note that you will have to build [the Python 2 shared-module](https://github.com/flathub/shared-modules/tree/master/python2.7) as it is not in any runtime.
* `--build-isolation`: Enable build isolation with pip (recommended but not always work).
* `--cleanup=(scripts|all)`: Add `cleanup` to the manifest. This is used when the packages installed are only used at build time.
* `--build-only`: Alias to `--cleanup=all`.
* `--requirements-file=`, `-r`: Reads the list of packages from `requirements.txt` file.
* `--ignore-pkg=`: Ignore a specific package name in a requirements-file, otherwise ignored.
* `--checker-data`: This adds `x-checker-data` to modules so you will be notified when new releases happen. See [flatpak-external-data-checker](https://github.com/flathub/flatpak-external-data-checker) for more details.
* `--runtime=`: Runs `pip` inside of a specific Flatpak runtime instead of on your host. Highly recommended for reproducability and portability. Examples would be `org.freedesktop.Sdk//22.08` or `org.gnome.Sdk/aarch64/43`.
* `--ignore-errors=`: Allow the generation of empty or otherwise broken files when downloading packages fails.
* `--ignore-installed=`: Comma-separated list of package names for which pip should ignore already installed packages. Useful when the package is installed in the SDK but not in the runtime.
* `--output=`: Sets an output file.
* `--yaml`: Outputs a YAML file.
