# SDK Extension for OCaml stable

This extension contains various components of the [OCaml](https://ocaml.org/) stable toolchain.

Includes the [Dune build system](https://dune.build/) and the OCaml Package Manager [opam](https://opam.ocaml.org/).
## Create Flatpak OCaml applications

Several examples on how to Flatpak OCaml applications leveraging this SDK can be found [here](https://github.com/josecastillolema/flatpak-ocaml-examples).

## Debugging

In order to use this extension in a Flatpak SDK environment you may add all provided tools in your PATH by executing first:
```
$ source /usr/lib/sdk/ocaml/enable.sh
```

## Install packages interactivelly

In order to interactivelly install [OCaml Package Manager (opam)](https://opam.ocaml.org/) packages in a Flatpak environmment you will need to inicialize a new environment:
```
$ opam init --disable-sandboxing --no-setup --root $XDG_DATA_HOME/ocaml
$ eval $(opam env --root=$XDG_DATA_HOME/ocaml --switch=default --set-root --set-switch)

$ opam switch
#  switch   compiler     description
→  default  ocaml.5.1.0  default
```

## Development
To use the SDK with your favourite editor:
```
$ sudo flatpak override --env=FLATPAK_ENABLE_SDK_EXT=ocaml com.visualstudio.code
$ sudo flatpak override --env=FLATPAK_ENABLE_SDK_EXT=ocaml org.gnu.emacs
$ sudo flatpak override --env=FLATPAK_ENABLE_SDK_EXT=ocaml io.neovim.nvim
```

or just use `FLATPAK_ENABLE_SDK_EXT=*` to load every SDK available in your system:
```
$ sudo flatpak override --env=FLATPAK_ENABLE_SDK_EXT=* com.visualstudio.code
```

When running your editor you should see something like this, i.e.:
```
$ flatpak run com.visualstudio.code
flatpak-vscode: Enabling SDK extension "ocaml"
```
