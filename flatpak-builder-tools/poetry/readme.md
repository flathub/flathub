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

## Development

1. Install Poetry v2 https://python-poetry.org/docs/#installation
2. `poetry install --with dev`
3. Format and lint: `poetry run ruff format && poetry run ruff check --fix --exit-non-zero-on-fix`
4. Type check: `poetry run mypy .`
