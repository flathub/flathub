# Elk

## Building

You can build and directly install the built Flatpak:

    flatpak-builder --install ./build ./zone.elk.Elk.json --force-clean -y

or export the Flatpak into a repo for later installation or bundling:

    flatpak-builder --repo ./repo ./build ./zone.elk.Elk.json --force-clean

Install from repository:

    flatpak install ./repo zone.elk.Elk

Export bundle:

    flatpak build-bundle ./repo ./Elk.flatpak zone.elk.Elk

## TODO

In no particular order:

 * [ ] Network Permission
 * [ ] XDG Config Permission
 * [ ] Correct name of logos/icons
 * [ ] Login currently fails with "Redirection to URL with a scheme that is not HTTP(S)"
 * [ ] Upstream metadata
 * [ ] Silence [nuxi build](https://nuxt.com/docs/api/commands/build) prerendering
 * [ ] Upstream `flatpak-pnpm-generator.py`
 * [ ] Document creation of the `-sources.json` (or `-sources.yaml`) files
 * [ ] Cleanup main manifest
 * [ ] Investigate building `pnpm` binary from source
 * [ ] Add [shared-modules/libappindicator](https://github.com/flathub/shared-modules/tree/master/libappindicator)
 * [ ] Point to elk-native release instead of a random commit
 * [ ] Investigate switching to YAML based manifest

