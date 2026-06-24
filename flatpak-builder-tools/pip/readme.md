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

## Source Selection

By default, this tool selects artifacts from PyPI using the following
priority: universal wheels (`none-any.whl`) > sdists.

If neither is available for a module, an error is raised. Platform
specific wheels are ignored unless explicitly enabled via
`--prefer-wheels=module1,module2,...`.

When `--prefer-wheels` is used, a Flatpak runtime must be provided with
the `--runtime` argument. The runtime must include `Python`, `pip`, and
the `python-packaging` module. This is used to determine platform tags
(Python version, ABI, and architecture).

By default, platform wheels are considered for the `x86_64` and
`aarch64`. This can be overridden with `--wheel-arches arch1,arch2,...`.

If the specified runtime is only available for a single architecture,
platform tags for other architectures are inferred from it.

## Examples for preferring platform wheels

### Generate for x86_64 and aarch64

```sh
./flatpak-pip-generator --runtime org.freedesktop.Sdk//25.08 --prefer-wheels=cryptography,cffi cryptography
```

### Generate for only x86_64

```sh
./flatpak-pip-generator --runtime org.freedesktop.Sdk//25.08 --prefer-wheels=cryptography,cffi --wheel-arches x86_64 cryptography
```

### Generate for x86_64 and ppc64le

```sh
./flatpak-pip-generator --runtime org.freedesktop.Sdk//25.08 --prefer-wheels=cryptography,cffi --wheel-arches x86_64,ppc64le cryptography
```

### Advanced usage

Artifact policy can be specified on a per-module basis, although this
option is generally not recommended. The available policies are
`universal`, `sdist`, and `platform`, corresponding to universal
wheels, source distributions, and platform-specific wheels,
respectively.

```
./flatpak-pip-generator --runtime org.freedesktop.Sdk//25.08 --artifact-policy cryptography=sdist --artifact-policy cffi=platform cryptography
```

## Options

```
./flatpak-pip-generator --help
flatpak-pip-generator

Tool to generate flatpak-builder manifests for Python modules

positional arguments:
  packages

options:
  -h, --help            Show this help message and exit
  --python2             Look for a Python 2 package
  --cleanup {scripts,all}
                        Select what to clean up after build
  --requirements-file, -r
                        Specify requirements.txt file. Cannot be used with pyproject file.
  --pyproject-file      Specify pyproject.toml file. Cannot be used with requirements file.
  --optdep-groups       Comma-separated optional dependency groups to include. Can only be used with pyproject file.
  --build-only          Clean up all files after build
  --build-isolation     Do not disable build isolation. Mostly useful on pip that does't support the feature.
  --ignore-installed    Comma-separated list of package names for which pip should ignore already installed packages. Useful when the package is installed in the SDK but not in the runtime.
  --checker-data        Include x-checker-data in output for the "Flatpak External Data Checker"
  --output, -o          Specify output file name
  --runtime             Specify a flatpak to run pip inside of a sandbox, ensures python version compatibility. Format: $RUNTIME_ID//$RUNTIME_BRANCH
  --yaml                Use YAML as output format instead of JSON
  --ignore-errors       Ignore errors when downloading packages
  --ignore-pkg          Comma-separated list of packages to ignore when generating the manifest. Include version constraints if present (e.g. --ignore-pkg 'foo>=3.0.0,baz>=21.0').
  --prefer-wheels       Comma-separated list of packages for which platform wheels should be preferred over sdists
  --wheel-arches        Comma-separated list of architectures for which platform wheels should be generated (default: x86_64,aarch64)
```

## Development

1. Install uv https://docs.astral.sh/uv/getting-started/installation/
2. `uv sync -v --all-groups --frozen`
3. Format and lint: `uv run ruff format && uv run ruff check --fix --exit-non-zero-on-fix`
4. Type check: `uv run mypy .`
