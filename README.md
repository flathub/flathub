# F-Droid Repomaker's Flatpak

This project contains the source files to create the
[FlatPak](https://flatpak.org/) for
[F-Droid Repomaker](https://f-droid.org/repomaker/).

With Repomaker, you can easily create your own
[F-Droid](https://f-droid.org) repo without needing any special
knowledge to do so.

## Installation

To install Repomaker from Flathub, use the following:

```bash
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user -y flathub org.fdroid.Repomaker
```

## Run

To run Repomaker with Flatpak, execute:

```bash
flatpak run org.fdroid.Repomaker
```

## Update

To update Repomaker from Flathub, use the following command:

```bash
flatpak update --user org.fdroid.Repomaker
```

## Development

To test the application locally, use
[flatpak-builder](http://docs.flatpak.org/en/latest/flatpak-builder.html)
with:

```bash
git clone https://gitlab.com/fdroid/fdroid-repomaker-flatpak.git
cd fdroid-repomaker-flatpak
git submodule update --init
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak-builder builddir --install-deps-from=flathub --user --install --force-clean --ccache org.fdroid.Repomaker.json
flatpak run org.fdroid.Repomaker
```

### Install latest dev flatpak

If you don't want to build the flatpak yourself,
but still want to use the latest version,
you can use the builds produced by GitLab CI.

Note that the artifacts get removed two weeks after the build,
so you might get a 404 because there aren't any artifacts.
Feel free to request a rebuild by [contacting us](https://f-droid.org/en/about/)!

```bash
curl -L https://gitlab.com/fdroid/fdroid-repomaker-flatpak/-/jobs/artifacts/master/download?job=flatpak -o repomaker-flatpak.zip
unzip repomaker-flatpak.zip
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak remote-add fdroid-repomaker-dev repomaker-flatpak --no-gpg-verify --user
flatpak install fdroid-repomaker-dev org.fdroid.Repomaker
flatpak run org.fdroid.Repomaker
```

After using it, you might want to uninstall the flatpak and
remove the repo:
```bash
flatpak remove org.fdroid.Repomaker
flatpak remote-delete fdroid-repomaker-dev
```

### Open bash inside flatpak

If you have Repomaker installed with flatpak, you can open a bash
inside the flatpak with the following command:

```bash
flatpak run --command=bash org.fdroid.Repomaker
```

Note that everything will look like if you're still on your own bash.
To check whether you're inside the flatpak,
you can list the content of `/app` which mostly only exists in flatpak
but not in your host OS.

```bash
ls /app
```

To quit the bash, simply run `exit`.

## License

Everything in this repo is licensed under GNU Affero General Public
License version 3.
See [LICENSE.md](LICENSE.md) for more information.
