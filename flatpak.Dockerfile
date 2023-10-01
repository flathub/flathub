FROM archlinux

RUN ln -s /usr/share/zoneinfo/UTC /etc/localtime

RUN pacman-key --init
RUN pacman -Sy archlinux-keyring --noconfirm && \
    pacman -Syu --noconfirm \
                            bash \
                            git \
                            python \
                            python-pipx \
                            python-aiohttp \
                            python-toml \
                            flatpak \
                            flatpak-builder

# list available runtimes with `flatpak remote-ls --user flathub | grep org.gnome.Sdk`

RUN flatpak install -y org.gnome.Platform//45 org.gnome.Sdk//45 runtime/org.freedesktop.Sdk.Extension.rust-stable/x86_64/23.08 runtime/org.freedesktop.Sdk.Extension.node18/x86_64/23.08

RUN git clone https://github.com/flatpak/flatpak-builder-tools.git /opt/flatpak-builder-tools

RUN echo 'export PATH=$PATH:/root/.local/bin' >> /root/.bashrc

RUN cd /opt/flatpak-builder-tools/node; pipx install .

RUN cp /opt/flatpak-builder-tools/cargo/flatpak-cargo-generator.py /usr/bin/flatpak-cargo-generator.py

RUN chmod +x /usr/bin/flatpak-cargo-generator.py

WORKDIR /
