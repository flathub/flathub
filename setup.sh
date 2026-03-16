#!/bin/bash

# Install the base Freedesktop SDK
flatpak install flathub org.freedesktop.Sdk//25.08

# Install the OpenJDK 25 extension for that SDK
flatpak install flathub org.freedesktop.Sdk.Extension.openjdk25//25.08