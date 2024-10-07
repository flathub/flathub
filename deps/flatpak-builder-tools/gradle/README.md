# Flatpak Gradle Generator

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
