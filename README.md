# Vala Stable Sdk

This extension to the ``org.freedesktop.Sdk`` provides
lots of tooling for the [Vala programming language](https://gitlab.gnome.org/gnome/vala):
 * the Vala compiler (valac) and libvala
 * the Vala API generator (vapigen)
 * the Vala API documentation tool (valadoc)
 * and the [Vala Language Server](https://github.com/prince781/vala-language-server) (vls)

## Usage

If you need any of these to build a Flatpak application,
add these lines to your Manifest:

```
"sdk-extensions" : [
    "org.freedektop.Sdk.Extension.vala"
]
...
"build-options" : {
    "prepend-path" : "/usr/lib/sdk/vala/bin/",
    "prepend-ld-library-path" : "/usr/lib/sdk/vala/lib"
}
```

## Example

```
{
    "id" : "org.example.MyApp",
    "runtime" : "org.freedesktop.Platform",
    "runtime-version" : "21.08",
    "sdk" : "org.freedesktop.Sdk",
    "sdk-extensions" : ["org.freedesktop.Sdk.Extension.vala"],
    "build-options" : {
        "append-path" : "/usr/lib/sdk/vala/bin",
        "prepend-ld-library-path" : "/usr/lib/sdk/vala/lib"
    },
    "modules" : [
        "name" : "Myapp",
    ]
}
```

## Debugging/Development

In order to use this extension in flatpak SDK environment
you may add all provided tools in your PATH by executing first:

``source /usr/lib/sdk/vala/enable.sh``

