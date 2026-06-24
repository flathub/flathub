There are two alternative methods to generate a Flatpak sources file for a
Gradle project:

1. Using a Gradle plugin
2. Using the `flatpak-gradle-generator.py` script

# Gradle plugin

Available on the [Gradle Plugin Portal](https://plugins.gradle.org/plugin/io.github.jwharm.flatpak-gradle-generator).

Add the plugin to your build:

```groovy
plugins {
  id "io.github.jwharm.flatpak-gradle-generator" version "1.5.0"
}
```

Configure the `flatpakGradleGenerator` task:

```groovy
tasks.flatpakGradleGenerator {
  outputFile = file("flatpak-sources.json")
  downloadDirectory = "./offline-repository"
  excludeConfigurations = ["testCompileClasspath", "testRuntimeClasspath"]
}
```

The `outputFile` is the Flatpak sources file containing the download urls of
all dependency artifacts. Flatpak-builder will download these files in the
`downloadDirectory` location. This directory will have a standard Maven
repository layout. Add it to your project `repositories`:

```groovy
repositories {
  mavenCentral()
  maven { url "./offline-repository" }
}
```

With this setup, Gradle will use the normal repositories such as Maven Central
during regular ("online") builds, and the "offline repository" in the
flatpak-builder sandbox environment.

Run the `flatpakGeneratorTask` to generate the "flatpak-sources.json" file.
When done, commit the file to your project repository and add it to the flatpak
manifest.

## Modular builds

Gradle plugins are not allowed to resolve configurations of other Gradle
(sub)projects. In a modular Gradle build, configure a `flatpakGradleGenerator`
task for each subproject individually.

## Plugin dependencies

Add the offline repository in the `settings.gradle` file to use it as a
plugin repository:

```groovy
pluginManagement {
  repositories {
    maven { url "./offline-repository" }
  }
}
```

This is necessary for all plugins that are not included in the Gradle
distribution directly, such as (for example) the Kotlin JVM plugin.

## Issues and contributions

Please log issues and contributions for the plugin in the GitHub
[jwharm/flatpak-gradle-generator](https://github.com/jwharm/flatpak-gradle-generator/)
repository.

# Flatpak Gradle Generator script

Tool to automatically generate a `flatpak-builder` sources file from a Gradle log.

## Requirements

You need to have `org.freedesktop.Sdk` and `org.freedesktop.Sdk.Extension.openjdk11` installed,
both branch 21.08.

## Usage

From withing the application's source directory, run:

```
flatpak run --command=bash --share=network --filesystem=`pwd` -d org.freedesktop.Sdk//21.08
```

This will enter the sandbox sharing just the current directory between your home directory
and the source application, then do what's needed to compile the application, for example,
for Ghidra:

```sh
$ source /usr/lib/sdk/openjdk11/enable.sh
$ rm -rf gradle-cache
$ mkdir -p dependencies/flatRepo/

# Install some ghidra specific files, should probably be installed by hand/outside this script
$ wget https://github.com/pxb1988/dex2jar/releases/download/2.0/dex-tools-2.0.zip
$ unzip -j dex-tools-2.0.zip "*.jar" -d dependencies/flatRepo/
$ wget -P dependencies/flatRepo/ https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/android4me/AXMLPrinter2.jar

# Launch gradle build with `--info` to log all the http(s) URLs
$ gradle -g gradle-cache/ --info --console plain buildGhidra > gradle-log.txt
```

Then exit the sandbox (Ctrl+D or `exit`), and parse the build log by running:

```
flatpak-gradle-generator.py gradle-log.txt gradle-dependencies.json
```

To make reproducing the build easier, we recommend that you create a `script.sh` with
the manual commands above to ship in your Flatpak repository, so you can run:

```
$ flatpak run --command=bash --share=network --filesystem=`pwd` -d org.freedesktop.Sdk//21.08 ./script.sh
$ flatpak-gradle-generator.py gradle-log.txt gradle-dependencies.json
```

If necessary, you can modify the level of output from the script by setting the
`LOGLEVEL` environment variable to a [supported log level](https://docs.python.org/3/library/logging.html#logging-levels):
```
$ LOGLEVEL=debug flatpak-gradle-generator.py gradle-log.txt gradle-dependencies.json
```
