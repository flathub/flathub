build:
    flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/media/ --repo=repo builddir io.github.ryubing.Ryujinx.yml

add-repo:
    flatpak --user remote-add --if-not-exists --no-gpg-verify local-ryubing ./repo

rm-repo:
    flatpak --user remote-delete local-ryubing

install: add-repo
    flatpak install --user --reinstall -y local-ryubing io.github.ryubing.Ryujinx

run: install
    flatpak run io.github.ryubing.Ryujinx

lint:
    flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest io.github.ryubing.Ryujinx.yml
    flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo

gen-sources csproj="../Ryujinx/Ryujinx.sln":
    flatpak-builder-tools/dotnet/flatpak-dotnet-generator.py -d 9 -f 24.08 tmp.json {{csproj}}
    cat tmp.json extra-sources.json | jq -s 'add' > nuget_sources.json
    rm tmp.json
