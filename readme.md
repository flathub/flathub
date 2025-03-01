# Rimsort Flatpak

## To install
`flatpak run org.flatpak.Builder --sandbox --user --force-clean --install --install-deps-from=flathub --repo=repo builddir io.github.rimsort.RimSort.yaml`

## To run
`flatpak run io.github.rimsort.RimSort`

## To debug

`flatpak --command=bash run io.github.rimsort.RimSort`

`flatpak run --command=sh --devel io.github.rimsort.RimSort`

## To make a bundle

`flatpak build-bundle repo hello.flatpak io.github.rimsort.RimSort --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo`

`flatpak build-bundle repo/ RimSortflatpak io.github.rimsort.RimSort`

## to linter

`flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest io.github.rimsort.RimSort.yaml`

`flatpak run --command=flatpak-builder-lint org.flatpak.Builder --exceptions repo repo`

## Locally testing the x-checker-data

Install flatpak-external-data-checker:

`flatpak install --from https://dl.flathub.org/repo/appstream/org.flathub.flatpak-external-data-checker.flatpakref`

Run flatpak-external-data-checker:

`flatpak run org.flathub.flatpak-external-data-checker io.github.rimsort.RimSort.yaml --update`


## To validate io.github.rimsort.RimSort.metadata

`appstreamcli validate io.github.rimsort.RimSort.metainfo.xml`
