# freelens-flatpak

The [flatpak](https://flathub.org/) package for
[Freelens](https://github.com/freelensapp/freelens).

## Usage

```sh
flatpak install app.freelens.Freelens
```

The application is sandboxed. It brings bundled `kubectl` and `helm` commands
and uses `~/.kube/config` file by default.

Flatpak adds wrapper for `aws`, `gke-gcloud-auth-plugin` and `kubelogin`
tools and runs them as a command from host system.

Terminal uses `/bin/sh` by default but it might be switched either to ie.
`/bin/bash` for sandboxed environment or `/app/bin/host-spawn` for host
environment.
