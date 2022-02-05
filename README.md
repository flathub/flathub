# MediathekView Flatpak

## Update maven-dependencies.json

Clone the MediathekView repository, switch to the relevant tag, and then run the following commands inside the clone:

```console
$ REPODIR=//path/to/a/fresh/directory
$ mkdir "$REPODIR"
$ ./mvnw -B -Dmaven.repo.local="$REPODIR" clean install > "$REPODIR/maven-output"
```

Then change into this directory and run:

```console
$ ./update-dependencies.py "$REPODIR"
```
