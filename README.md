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
