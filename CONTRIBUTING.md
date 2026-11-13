# Contributing

## Contributing (Project)

Refer to [project's contributing guidelines](https://gitlab.com/lotconn/rukn/CONTRIBUTING.md)

## AI Contribution Policy

We aim to be transparent about contributions from AI agents, LLMs, and bots. AI-assisted
contributions by humans are welcome. Any other autonomous AI contributions — including merge/pull
requests, issues, and comments — will be rejected. This includes any "Signed-off-by:
`<agent's operator>`" variants, i.e. human-operated agentic submissions. Human contributors may
include an "Assisted by: " footer in any contribution that involved AI or LLM assistance but is not
required.

## Building from Source

Clone the repo:

```bash
git clone --recursive --depth 1 https://gitlab.com/lotconn/rukn.git ./rukn_lotconn \
  && cd ./rukn_lotconn
```

### Supported Platforms

- x86_64
- arm64

### Available Build Flags

| Option              | Default   | Available            | Description                                                                                                         |
| ------------------- | --------- | -------------------- | ------------------------------------------------------------------------------------------------------------------- |
| `APP_LINUX_DIST`    | `Default` | `Default`, `Flatpak` | Sets build-type and activates quirks accordingly. `Flatpak` is automatically set when flatpak building is detected. |
| `BUILD_SHARED_LIBS` | `OFF`     | `ON`, `OFF`          | Sets build-type of third-party libraries. Recommended to keep at default                                            |

### 1. Standalone development on IDE

We recommend using an IDE over plugin-loaded text-editors (TUI or not). The repository already
contains some essential auto-formatting and linting configurations for improving development
experience (DX). VSCode, VSCodium and Qt Creator are highly recommended, although you're free to
choose your preferred program. For UI developments especially, Qt Design Studio is highly preferred.

> [!Warning]
>
> `org.kde.Platform` and `org.kde.Sdk` versions older than 6.11 are not compatible. Build with
> project's latest tracking KDE platforms version before filing bugs.

#### Recommended Linters and Formatters

- [Prettier](https://prettier.io/docs/install)

### Building

The recommended method is to use `org.flatpak.Builder`:

```bash
flatpak install -y flathub org.flatpak.Builder

# Add the flathub repo user-wide
flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo

# Then build the manifest
flatpak run --command=flathub-build org.flatpak.Builder --install org.lotconn.Rukn.yml

# Run the app (provided no previous installation of flatpak Rukn is present)
flatpak run org.lotconn.Rukn

# Or more specifically
flatpak run --user --branch master --origin rukn-origin org.lotconn.Rukn
```

<details><summary>Further Developer Notes (Click to expand)</summary>

To build with `flatpak-builder`:

```bash
flatpak-builder \
--verbose \
--force-clean \
--user \
--install-deps-from=flathub \
--ccache \
--repo=./repo ./builddir ./org.lotconn.Rukn.yml

# Run the built app
flatpak-builder --run \
--env=WAYLAND_DISPLAY="$WAYLAND_DISPLAY" \
./builddir ./org.lotconn.Rukn.yml rukn
```

#### Known Issues

- Few core & UI features like adding custom adhan doesn't work as expected due to flatpak sandboxing
  when using `flatpak-builder`.

</details>

### Guidelines

#### Recommended First-Reads

- Flatpak documentation: <https://docs.flatpak.org/en/latest/>
- Flathub documentation: <https://docs.flathub.org/docs/category/for-app-authors>

#### Versioning

Version format follows `YY.MM` convention (e.g., `21.04` for April 2021). Subsequent releases within
same month follows `YY.MM.m` convention (e.g., `21.04.1` for another release on April 2021).

#### General

- Maintain [versioning scheme](#versioning)
- Update `CHANGELOG.md` and XDG metainfo with notable changes.
- Suggested to set these environment variables on app executions for extended-yet-sane level of
  logging:
  - `QT_MESSAGE_PATTERN="[%{time process} %{type}] %{category} - %{message}"`
  - `QT_LOGGING_RULES="*.debug=true;qt.*.debug=false;qt.*.info=true;kf.*.debug=false"`

#### Validations

- Validate your manifest first:

  ```bash
  flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest /path/to/your/org.lotconn.Rukn.metainfo.xml
  ```

  > [!Note]
  >
  > Make sure your manifest uses
  > [latest tagged commit of the project](https://gitlab.com/lotconn/rukn/-/tags) as the source. Do
  > not use other commits as those are not guaranteed to be stable.

- Then your AppStream metainfo:

  ```bash
  flatpak run --command=flatpak-builder-lint org.flatpak.Builder metainfo /path/to/your.metainfo.xml
  ```

- Then test build and install [following the recommended workflow](#building). Replace with your
  manifest.

- Additionally, check your repo after building

  ```bash
  flatpak run --command=flatpak-builder-lint repo repo
  ```

---

After passing all validations and ensuring guidelines are followed,
[submit a merge request](https://github.com/flathub/org.munadi.Munadi/pull/new) to master branch
with a brief summary of the MR. Only include necessary points that make the MR noteworthy.
