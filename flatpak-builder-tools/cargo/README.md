# flatpak-cargo-generator

Tool to automatically generate `flatpak-builder` manifest json from a `Cargo.lock`.

## Requirements:
- siphash
- Python 3


## Usage:

The first step is to convert the locked depenencies by Cargo into a format flatpak-builder can understand
```
python3 ./flatpak-cargo-generator.py ./quickstart/Cargo.lock -o cargo-sources.json
```

The output file should be added to the manifest like
```json
{
    "name": "quickstart",
    "buildsystem": "simple",
    "build-commands": [
        "cargo --offline fetch --manifest-path Cargo.toml --verbose",
        "cargo --offline build --release --verbose",
        "install -Dm755 ./target/debug/quickstart -t /app/bin/"
    ],
    "sources": [
        {
            "type": "dir",
            "path": "."
        },
        "cargo-sources.json"
    ]
}
```

Make sure to override CARGO_HOME env variable to point it to `/run/build/$module-name/cargo` where `$module-name` is the flatpak module name, `quickstart` in this example.


For a complete example see the quickstart project.