# tech.rethunk.Mortar

Flathub packaging for [Mortar](https://github.com/Rethunk-Tech/mortar).

## Releasing a new version

Point the manifest at a Mortar release tag and regenerate the offline sources:

```sh
./scripts/regen-sources.sh vX.Y.Z
```

The script clones that tag and runs the `build/linux/flathub/render.sh` it ships, then copies the manifest,
`go-sources.json`, `node-sources.json` and `yarn.lock` it writes over the ones here. It needs network, go, bun, jq
and uvx. Run against v0.6.0 it reproduces the manifest, `go-sources.json` and `node-sources.json`
byte for byte; `yarn.lock` differs only in its `# bun ./bun.lockb --hash` comment, which newer bun writes as zeros.

Commit those files and open the Flathub pull request.
