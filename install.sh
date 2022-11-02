flatpak remote-delete ChartGeanyRepo
flatpak remote-add ChartGeanyRepo ChartGeanyRepo --no-gpg-verify --if-not-exists
flatpak install ChartGeanyRepo io.sourceforge.chart_geany.chart_geany
