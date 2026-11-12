# tech.rethunk.Mortar

Flathub packaging for [Mortar](https://github.com/Rethunk-Tech/mortar).

## Releasing a new version

Point the manifest at a Mortar release tag and regenerate the offline sources:

```sh
./scripts/regen-sources.sh vX.Y.Z
```

The script fetches that tag's `go.mod`/`go.sum` and lockfile from
Rethunk-Tech/mortar, runs `flatpak-go-mod` and `flatpak-node-generator` (via
`uvx` or `bunx`), writes `go-sources.json` and `node-sources.json`, and updates
the git tag and commit in `tech.rethunk.Mortar.yml`.

Commit those files and open the Flathub pull request.
