# Flatpak Deno Generator

Run from jsr

```
deno -RN -W=. jsr:@flatpak-contrib/flatpak-deno-generator deno.lock
```

or locally from this repo

```
deno -RN -W=. src/main.ts deno.lock --output sources.json
```

This will create a `deno-sources.json` (or the name specified with --output)
that can be used in flatpak build files. The sources files provides these 2
directories:

- it creates and populates `./deno_dir` with npm dependencies
- it creates and populates `./vendor` with jsr + http dependencies

## Usage:

- Use the sources file as a source, example:

```yml
sources:
  - deno-sources.json
```

- To use `deno_dir` (when your project have npm dependencies) point `DENO_DIR`
  env variable to it, like so:

```yml
- name: someModule
  buildsystem: simple
  build-options:
    env:
      # sources provides deno_dir directory
      DENO_DIR: deno_dir
```

- To use `vendor` (when your project have http or jsr dependencies) move it next
  to your `deno.json` file and make sure to compile or run with `--vendor` flag,
  exmaple:

```yml
- # sources provides vendor directory
- # src is where my deno project at as in deno.json is under src directory, so I'm moving vendor next to it
- mv ./vendor src/
- DENORT_BIN=$PWD/denort ./deno compile --vendor --no-check --output virtaudio-bin --cached-only
  --allow-all --include ./src/gui.slint --include ./src/client.html ./src/gui.ts
```

## Notes

Currently this only supports lockfile V5 (available since deno version 2.3)

## License

MIT

## Example

- checkout https://github.com/flathub/io.github.sigmasd.VirtAudio/

## Technical Info

Theoretically it would've been better to put all the dependencies in `DENO_DIR`
but currently thats not possible because jsr and https dependencies have some
special metadata checks made by deno, more info here
https://github.com/denoland/deno/issues/29212
