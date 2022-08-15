# Pass arguments (e.g. change database location)
## From shell
```
flatpak run --command=hydrus_client io.github.hydrusnetwork.hydrus --db_dir=/home/ben/.local/share/hydrus/db
```
## From desktop entry
```
[Desktop Entry]
[...]
Exec=/usr/bin/flatpak run --command=hydrus_client --file-forwarding io.github.hydrusnetwork.hydrus --db_dir=~/path/to/db  @@u %U @@
[...]
```