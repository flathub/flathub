# Flatpak PIP Generator

Tool to automatically generate `flatpak-builder` manifest json from a `pip`
package-name. Requires `requirements-parser`.

## Usage

`flatpak-pip-generator foo` which generates `python3-foo.json` and can be included in a manifest like:

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
flatpak-pip-generator foo\>=1.0.0,\<2.0.0 bar 
```

If your project contains a [requirements.txt file](https://pip.readthedocs.io/en/stable/user_guide/#requirements-files) with all the project dependencies, you can use 
```
flatpak-pip-generator --requirements-file=/the/path/to/requirements.txt --output pypi-dependencies
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
