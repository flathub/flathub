# must have a checkout from the cigale app of the right version in ../cigale
mkdir .cargo
cargo vendor > .cargo/config
rm flatpak-cargo-generator.py
wget https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/d7cfbeaf8d1a2165d917d048511353d6f6e59ab3/cargo/flatpak-cargo-generator.py
python3 flatpak-cargo-generator.py ../cigale/Cargo.lock -o cargo-sources.json
