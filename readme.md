# Rimsort Flatpak

## To install
flatpak run org.flatpak.Builder --sandbox --user --force-clean --install --install-deps-from=flathub  --repo=repo builddir org.RimSort.RimSort.yaml

## To run
flatpak run org.RimSort.RimSort

## To debug

flatpak --command=bash run org.RimSort.RimSort
