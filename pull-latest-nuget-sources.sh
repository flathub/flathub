#!/bin/bash
curl -Lo nuget-sources.json.zip https://nightly.link/hedge-dev/HedgeModManager/workflows/build-project/main/nuget-sources.json.zip
rm nuget-sources.json
unzip nuget-sources.json.zip
rm nuget-sources.json.zip