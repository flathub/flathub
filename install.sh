flatpak remote-delete ChartGeanyRepo
flatpak remote-add ChartGeanyRepo ChartGeanyRepo --no-gpg-verify --if-not-exists
flatpak install ChartGeanyRepo io.sourceforge.chart-geany.chart-geany
