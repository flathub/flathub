This repository contains commonly shared modules and is intended to be used as a git submodule.

To use shared modules for packaging an application, add the submodule:

```
git submodule add https://github.com/flathub/shared-modules.git
```

Then modules from this repository can be specified in a manifest JSON file like this:

```json
"modules": [
  "shared-modules/SDL/SDL-1.2.15.json",
  {
    "name": "foo"
  }
]
```

[See the description in the Flathub wiki](https://github.com/flathub/flathub/wiki/App-Requirements#shared-modules) for more information.

Please do not request adding modules unless they have many users in the Flathub repository.
