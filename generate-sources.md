### Clone flatpak tools

`git clone --depth 1 https://github.com/flatpak/flatpak-builder-tools.git`

### Generate cache for nuget

`git clone --depth 1 -b v10.8.9 https://github.com/jellyfin/jellyfin.git`

`./flatpak-builder-tools/dotnet/flatpak-dotnet-generator.py --runtime=linux-x64 nuget-generated-sources-x64.json jellyfin/Jellyfin.Server/Jellyfin.Server.csproj`

`./flatpak-builder-tools/dotnet/flatpak-dotnet-generator.py --runtime=linux-arm64 nuget-generated-sources-arm64.json jellyfin/Jellyfin.Server/Jellyfin.Server.csproj`

### Generate cache for npm

`git clone --depth 1 -b v10.8.9 https://github.com/jellyfin/jellyfin-web.git`

`pip install ./flatpak-builder-tools/node`

`.local/bin/flatpak-node-generator -o npm-generated-sources.json npm jellyfin-web/package-lock.json`

### Remove source clones
`rm -rf {jellyfin,jellyfin-web,flatpak-builder-tools}`
