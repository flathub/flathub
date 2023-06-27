# 🏪 Emoji Mart popup picker

Flatpak for [github.com/vemonet/EmojiMart](https://github.com/vemonet/EmojiMart)

## 🛠️ Development

You might want to create a virtual env for the python dependencies used to generate the sources files:

```bash
# Create it
python -m venv .venv
# Activate it
source .venv/bin/activate
```

Clone this repository, and the EmojiMart repository in the same folder:

```bash
git clone --recursive https://github.com/vemonet/flathub -b io.github.vemonet.EmojiMart
git clone https://github.com/vemonet/EmojiMart
cd flathub
```

Install dependencies:

```bash
make install
```

Generate `cargo-sources.json` and `node-sources.json`:

```bash
make gen
```

Build flatpak:

```bash
make flatpak
```

Clean the cache:

```bash
make clean
```

## 📋 Notes

Examples tauri to flathub:
* https://github.com/flathub/in.cinny.Cinny/blob/master/in.cinny.Cinny.yml
* https://github.com/hadess/flathub/blob/d4b53ff829e0917c5129294132f619e5f12d337c/io.github.neil_morrison44.pocket-sync.yaml
