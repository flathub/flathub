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

# for some reason 21.08 == 42!?

RUN flatpak install -y org.gnome.Platform//42 org.gnome.Sdk//42 runtime/org.freedesktop.Sdk.Extension.rust-stable/x86_64/21.08 runtime/org.freedesktop.Sdk.Extension.node16/x86_64/21.08


RUN git clone https://github.com/flatpak/flatpak-builder-tools.git /opt/flatpak-builder-tools

RUN echo 'export PATH=$PATH:/root/.local/bin' >> /root/.bashrc

RUN cd /opt/flatpak-builder-tools/node; pipx install .

RUN cp /opt/flatpak-builder-tools/cargo/flatpak-cargo-generator.py /usr/bin/flatpak-cargo-generator.py

RUN chmod +x /usr/bin/flatpak-cargo-generator.py

WORKDIR /
