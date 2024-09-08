#!/bin/bash
flatpak-builder --repo=repo --force-clean build-dir flatpak/io.github.MrPiggy92.ScriptedJourneys.json
flatpak build-bundle repo ScriptedJourneys.flatpak io.github.MrPiggy92.ScriptedJourneys

