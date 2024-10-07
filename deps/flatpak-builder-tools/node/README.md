# flatpak-node-generator

A more modern successor for flatpak-npm-generator and flatpak-yarn-generator, for Node 10+ only.
(For Node 8, use flatpak-npm-generator and flatpak-yarn-generator.)

**NOTE:** `--xdg-layout` was recently changed to be the default. In the stark
majority of cases, this needed to be passed, so you can now omit using it
explicitly. If you're on a relatively old Electron version before this was
required, however, you can disable it explicitly via `--no-xdg-layout`.

## Requirements

- flatpak-builder 1.1.2 or newer
- Python 3.8+.
- [pipx](https://pypa.github.io/pipx/) (recommended) or
  [pip](https://pip.pypa.io/en/stable/) (both of these are usually available in
  your distro repositories, the latter is often included with Python installs).

## Usage

The easiest way to use this tool is to install it via
[pipx](https://pypa.github.io/pipx/), the officially recommended way of
installing Python packages, by running the following from this directory:

```
$ pipx install .
```

## Complete examples

There are two examples provided for how to use flatpak-node-generator:

- `vanilla-quick-start` - A Flatpak of
  [electron-quick-start](https://github.com/electron/electron-quick-start). It uses npm for
  package management and a rather basic Electron workflow.
  (Current on the Electron 4 version.)
- `webpack-quick-start` - A Flatpak of
  [electron-webpack-quick-start](https://github.com/electron-userland/electron-webpack-quick-start).
  It uses yarn for package management and electron-builder + webpack.

Both manifests have comments to highlight their differences, so you can mix and match to e.g.
get npm with electron-builder.

## Usage

```
usage: flatpak-node-generator [-h] [-o OUTPUT] [-r] [-R RECURSIVE_PATTERN]
                              [--registry REGISTRY] [--no-trim-index]
                              [--no-devel] [--no-requests-cache]
                              [--max-parallel MAX_PARALLEL]
                              [--retries RETRIES] [-P] [-s]
                              [--node-chromedriver-from-electron NODE_CHROMEDRIVER_FROM_ELECTRON]
                              [--electron-ffmpeg {archive,lib}]
                              [--electron-node-headers]
                              [--nwjs-version NWJS_VERSION]
                              [--nwjs-node-headers] [--nwjs-ffmpeg]
                              [--no-xdg-layout]
                              {npm,yarn} lockfile

Flatpak Node generator

positional arguments:
  {npm,yarn}
  lockfile              The lockfile path (package-lock.json or yarn.lock)

options:
  -h, --help            show this help message and exit
  -o OUTPUT, --output OUTPUT
                        The output sources file
  -r, --recursive       Recursively process all files under the lockfile
                        directory with the lockfile basename
  -R RECURSIVE_PATTERN, --recursive-pattern RECURSIVE_PATTERN
                        Given -r, restrict files to those matching the given
                        pattern.
  --registry REGISTRY   The registry to use (npm only)
  --no-trim-index       Don't trim npm package metadata (npm only)
  --no-devel            Don't include devel dependencies (npm only)
  --no-requests-cache   Disable the requests cache
  --max-parallel MAX_PARALLEL
                        Maximium number of packages to process in parallel
  --retries RETRIES     Number of retries of failed requests
  -P, --no-autopatch    Don't automatically patch Git sources from
                        package*.json
  -s, --split           Split the sources file to fit onto GitHub.
  --node-chromedriver-from-electron NODE_CHROMEDRIVER_FROM_ELECTRON
                        Use the ChromeDriver version associated with the given
                        Electron version for node-chromedriver
  --electron-ffmpeg {archive,lib}
                        Download prebuilt ffmpeg for matching electron version
  --electron-node-headers
                        Download the electron node headers
  --nwjs-version NWJS_VERSION
                        Specify NW.js version (will use latest otherwise)
  --nwjs-node-headers   Download the NW.js node headers
  --nwjs-ffmpeg         Download prebuilt ffmpeg for current NW.js version
  --no-xdg-layout       Don't use the XDG layout for caches

```

flatpak-node-generator.py takes the package manager (npm or yarn), and a path to a lockfile for
that package manager. It will then write an output sources file (default is generated-sources.json)
containing all the sources set up like needed for the given package manager.

If you're on npm and you don't want to include devel dependencies, pass --no-devel, and pass
--production to `npm install` itself when you call.

### Caching

flatpak-node-generator will cache many API responses and archives from the server to speed up
subsequent runs. You can disable this using `--no-requests-cache`, and it can be cleared via
`rm -rf ${XDG_CACHE_HOME:-$HOME/.cache}/flatpak-node-generator`.

### Splitting mode

If your Node app has too many dependencies (particularly with npm), the generated-sources.json
may be larger than GitHub's maximum size. In order to circumvent this, you can pass `-s`, which
will write multiple files (generated-sources.0.json, generated-sources.1.json, etc) instead of
one, each smaller than the GitHub limit.

### ChromeDriver support

#### node-chromedriver

If your app depends on node-chromedriver, then flatpak-node-generator will download it
to the directory `$FLATPAK_BUILDER_BUILDDIR/flatpak-node/chromedriver`. You need to
do two things in order to utilize this:

- Add `CHROMEDRIVER_SKIP_DOWNLOAD=true` to your environment variables.
- Add `$FLATPAK_BUILDER_BUILDDIR/flatpak-node/chromedriver` to your PATH.

It might look like this:

```yaml
build-options:
  append-path: '/usr/lib/sdk/node10/bin:/run/build/MY-MODULE/flatpak-node/chromedriver'
  env:
    CHROMEDRIVER_SKIP_DOWNLOAD: 'true'
    # ...
```

In addition, the default ChromeDriver only is available for x64. If you need to build
on other platforms, you can use the ChromeDriver binaries that are compiled by Electron
and distributed with their releases. To do this, pass
`--node-chromedriver-from-electron AN_ELECTRON_VERSION` to use the ChromeDriver given with
that Electron version. Note that you may not necessarily want to use a version here that
corresponds to the Electron version your app is using; many apps stay on older Electron
versions but may use newer ChromeDriver functionality.

#### electron-chromedriver

electron-chromedriver will be handled automatically, but make sure `ELECTRON_CACHE` is
set as show in the quickstart examples.

### Recursive mode

Sometimes you might have multiple lockfiles in a single source tree that need to have sources
generated for them. For this, you can pass `-r`, which will find all the lockfiles with the
name of the lockfile path you gave it in the same directory.

E.g. for instance, if you run:

```
flatpak-node-generator yarn -r ~/my-project/yarn.lock
```

flatpak-node-generator will find all files named yarn.lock inside of my-project.

If you want to match only certain lockfiles, pass `-R pattern` too:

```
flatpak-node-generator yarn -r ~/my-project/yarn.lock -R 'something*/yarn.lock' -R 'another*/yarn.lock'
```

In this case, only lockfiles matching `something*/yarn.lock` or `another*/yarn.lock` will be used.

With yarn, we're done here. However, npm has a few more curveballs you need to know about.

If you have any Git sources in your package.json, then they need to be patched to point to the
Flatpak-downloaded Git repos. flatpak-node-generator normally takes care of this patching
automatically. However, in the case of recursive package.jsons, this is a little different.

Say you have the following project directory structure:

```
my-project/
  node_modules/
    my-nested-project/
      package.json
      package-lock.json
  package-lock.json
```

`my-nested-project` doesn't ship built dependencies, so you need to build them yourself.
Therefore, you might run something like this in your Flatpak build commands:

- `npm install ...` in the root directory.
- `npm install ...` in the my-nested-project directory.
- `npm run build` or whatever build command in the my-nested-project directory.

However, if my-nested-project uses a Git source, then flatpak-node-generator will try to patch
it out...except my-nested-project's directory won't exist until you run the first `npm install`,
therefore the patch command and your build will fail.

In order to work around this, you need to pass `-P` / `--no-autopatch` to flatpak-node-generator.
This will disable the automated patching. Then, you'll need to call the scripts to patch your
package files manually. so a new build-commands might look like this:

- `flatpak-node/patch.sh`.
- `npm install ...` in the root directory.
- `flatpak-node/patch/node_modules/my-nested-project.sh`
- `npm install ...` in the my-nested-project directory.
- `npm run build` or whatever build command in the my-nested-project directory.

In short, flatpak-node-generator will generate a patch script named
`flatpak-node/patch/path-to-package-lock.json`; if package-lock.json is in the root directory,
then the name will just be `patch.sh`. Here these will be called manually, thereby ensuring
that the files that need to be patched will already exist.

(In addition, flatpak-node-generator will generate `flatpak-node/patch-all.sh`, which is what is
automatically run by default when you *don't* pass `-P`.)

### electron-builder and ARM architectures

If you want to build for ARM or ARM64 with electron-builder, there are two important
things to note:

- For ARM in particular, electron-builder will misdetect the architecture and give
  an error about it being unsupported. To work around this, you have to pass the
  architecture manually to electron-builder. flatpak-node-generator will create a script by adding
  it as an entry to the sources file. During the build process script will be created at
  `flatpak-node/electron-builder-arch-args.sh` so it can be sourced to set the
  `$ELECTRON_BUILDER_ARCH_ARGS` environment variable. Then, this variable can be passed to the
  electron-builder command.
- For both ARM and ARM64, the electron-builder output directory will contain the
  architecture in its name.

Both of these cases are handled by the electron-webpack-quick-start example.

### node-gyp and native dependencies

Some node/electron versions are binary incompatible and require rebuilding of
native node dependencies for electron. In offline mode, it may result in broken
ABI. If you are seeing errors like `The module 'something.node' was compiled
against a different Node.js version`, then pass `--electron-node-headers`
option to flatpak-node-generator and set `npm_config_nodedir` to
`flatpak-node/node-gyp/electron-current`.

**Note**: Setting `npm_config_nodedir` should not be necessary when using XDG-compliant
cache directories layout (the default, unless disabled via `--no-xdg-layout`).

Some tools like *electron-rebuild* don't properly respect the
XDG spec however. In this case, as a workaround, you might need to symlink the
cache directory. For example:

```yaml
build-commands:
  - |
    ln -s $XDG_CACHE_HOME/node-gyp $HOME/.electron-gyp
    npm run build
```

(Note that the build command must be ran as part of the same command as `ln`,
i.e. it won't work if you run them as separate commands.)

### ffmpeg support

If your app needs separate ffmpeg for matching electron version, add
`--electron-ffmpeg=archive` option to flatpak-node-generator. This will put
`ffmpeg-$suffix.zip` alongside electron in the cache directory.

By defualt, the ffmpeg that Electron ships with has proprietary codecs built in
like AAC and H.264. If you don't need these, you can pass
`--electron-ffmpeg=lib` to flatpak-node-generator. This will download
a patent-clean ffmpeg binary to `flatpak-node/libffmpeg.so`, which you can then
use to overwrite the default Electron ffmpeg, e.g.:

```yaml
- 'install -Dm 755 flatpak-node/libffmpeg.so -t /app/electron-webpack-quick-start'
```

An short example of this is again in the electron-webpack-quick-start

## NW.js

This scripts assumes NW.js is used if the lockfile contains `nw-builder` package.

### Specifying NW.js version

Unlike Electron, NW.js engine version is not reflected in NPM package.

- If the app you're building uses specific NW.js version, specify it
  using `--nwjs-version` argument
- If any NW.js version will suffice, this script will use latest;
  the version number will be stored in `flatpak-node/nwjs-version` file.
  You can tell `nw-builder` to use this version by passing `-v` arg to `nwbuild`:
  ```bash
  nwbuild -v $(<$FLATPAK_BUILDER_BUILDDIR/flatpak-node/nwjs-version)
  ```

## Contributing

We use [Poetry](https://python-poetry.org/) for local development. You can set up the
local virtualenv via:

```
$ poetry install
```

After making any changes, you can re-run all the checks & unit tests via:

```
$ poetry run poe check
```

or invoke pytest manually:

```
$ poetry run pytest -n auto
```

Note that these tests can take up quite a bit of space in /tmp, so if you hit `No space
left on device` errors, try expanding `/tmp` or changing `$TMPDIR`.

### Utility Scripts

A few utility scripts are included in the `tools` directory:

- `lockfile-utils.sh` has a few different helpers for working with the lockfiles used
  by test packages in `tests/data/packages`:
  - `lockfile-utils.sh update-lockfile PACKAGE-MANAGER PACKAGE` will recreate the
    lockfile for the given package manager (one of `npm-14` for Node 14's NPM, `npm-16`
    for Node 16's npm, or `yarn`).
  - `lockfile-utils.sh peek-cache PACKAGE-MANAGER PACKAGE` will install the dependencies
    from the corresponding lockfile and then extract the resulting package cache (npm)
    or mirror directory (yarn), for closer examination.
- `b64-to-hex.sh` will convert a base64 hash value from npm into hex, e.g.:
  ```
  $ echo x+sXyT4RLLEIb6bY5R+wZnt5pfk= | tools/b64-to-hex.sh
  c7eb17c93e112cb1086fa6d8e51fb0667b79a5f9
  ```
- `hex-to-b64.sh` will convert a hex hash value into base64, e.g.:
  ```
  $ echo 867ac74e3864187b1d3d47d996a78ec5c8830777 | tools/hex-to-b64.sh
  hnrHTjhkGHsdPUfZlqeOxciDB3c=
  ```
  For convenience, any slashes inside the hex value will be removed, allowing you to
  copy-paste a path into the npm package cache and still get the base64 value:
  ```
  $ echo c7eb17c93e112cb1086fa6d8e51fb0667b79a5f9 | tools/hex-to-b64.sh
  x+sXyT4RLLEIb6bY5R+wZnt5pfk=
  $ echo c7/eb/17c93e112cb1086fa6d8e51fb0667b79a5f9 | tools/hex-to-b64.sh
  x+sXyT4RLLEIb6bY5R+wZnt5pfk=
  ```
- `b64-integrity.sh INTEGRITY` will run `${INTEGRITY}sum` and then convert its output
  into base64, e.g.:
  ```
  $ echo 123 | tools/b64-integrity.sh sha512
  6i/la7jB+1rahJY7Qu1xt2SnSwktdXVRc63gby9KranADWwwLhhQNcvoX9/zFpi8qT6GYfDLzvUs8v9lhk/XQg==
  ```
  This is roughly equivalent to `echo 123 | sha512sum`, then converting the resulting
  hex digest to base64 via `hex-to-b64.sh`.
