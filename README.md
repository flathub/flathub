# MediathekView Flatpak

## Update maven-dependencies.json

Download the relevant MediathekView source tarball, extract it and run the following commands in the extracted directory:

```console
$ REPODIR=//path/to/a/fresh/directory
$ mkdir "$REPODIR"
$ ./mvnw -B -Dmaven.repo.local="$REPODIR" clean install > "$REPODIR/maven-output"
```

Then change into the directory containing your clone of this flatpak manifest and run:

```console
$ ./update-dependencies.py "$REPODIR"
```
