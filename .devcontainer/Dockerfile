FROM mcr.microsoft.com/devcontainers/base:ubuntu-20.04

ENV DEBIAN_FRONTEND=noninteractive


# ------------------------ Prepare docker environment ------------------------ #

RUN apt-get update && \
    # Install general utilities
    apt-get -y install tree && \
    # Install app-specific dependencies
    apt-get -y install keybinder-3.0 appindicator3-0.1 libappindicator3-1 libappindicator3-dev libnotify-dev && \
    # Install Flatpak dependencies
    apt-get -y install flatpak flatpak-builder

# Prepare environment for building Flatpak
RUN flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
RUN flatpak install -y org.gnome.Sdk/x86_64/44
RUN flatpak install -y org.gnome.Platform/x86_64/44
RUN flatpak install -y flathub org.freedesktop.appstream-glib
