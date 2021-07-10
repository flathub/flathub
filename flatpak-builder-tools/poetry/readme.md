# Flatpak Python Poetry Lockfile Generator

Tool to automatically generate `flatpak-builder` manifest json from a poetry.lock file.

## Usage

`flatpak-poetry-generator poetry.lock` which generates
`generated-poetry-sources.json` and can be included in a manifest like:

```json
"modules": [
  "generated-poetry-sources.json",
  {
    "name": "other-modules"
  }
]
```

## Optional Arguments
- `--production` does not include development dependencies
- `-o outputfile` to output to a different filename


