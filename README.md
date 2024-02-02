# org.jjazzlab.JJazzLab

Flatpak for [JJazzLab](https://github.com/jjazzboss/JJazzLab).

## Check Java version

Look at JJazzLab's `jjazzlab.compiler.release` property in its `pom.xml`.
Make sure it corresponds to the OpenJDK extension in use.

## Updating Maven dependencies

The build process does not have network access (by design). All files required need to be declared, and are downloaded by the builder.
For Maven, there are many files. After an application update, the list can be re-generated as follows.

1. In `org.jjazzlab.JJazzLab.yml`, enable network in build-args, disable `maven-dependencies.yml` source (see comments).

2. Build from scratch:
   ```
   flatpak-builder build-dir org.jjazzlab.JJazzLab.yml --force-clean --build-only --keep-build-dirs
   ```

3. Rebuild dependencies file (based on [this idea](https://github.com/flatpak/flatpak-builder/issues/58#issuecomment-781508777)):
   ```
   MAVEN_REPO="$PWD/.flatpak-builder/build/jjazzlab/.m2/repository"
   find $MAVEN_REPO \( -iname '*.jar' -o -iname '*.pom' -o -iname '*.nbm' \) -printf '%P\n' | sort -V | xargs -rI '{}' bash -c "echo -e \"- type: file\n  dest: .m2/repository/\$(dirname {})\n  url: https://repo.maven.apache.org/maven2/{}\n  sha256: \$(sha256sum \"$MAVEN_REPO/{}\" | cut -c 1-64)\"" >maven-dependencies.yml
   find $MAVEN_REPO -name maven-metadata-central.xml -printf '%P\n' | sort -V | xargs -rI '{}' bash -c "DL=\$(echo {} | sed 's/-central\.xml\$/.xml/') && echo -e \"- type: file\n  dest: .m2/repository/\$(dirname {})\n  dest-filename: maven-metadata-central.xml\n  url: https://repo.maven.apache.org/maven2/\$DL\n  sha256: \$(sha256sum \"$MAVEN_REPO/{}\" | cut -c 1-64)\"" >>maven-dependencies.yml
   ```

4. Revert temporary changes to `org.jjazzlab.JJazzLab.yml`.


