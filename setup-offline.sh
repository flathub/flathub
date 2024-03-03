#!/usr/bin/env bash
mkdir .build/repositories
cd .build/repositories
mkdir ./Adwaita-39caa65a
cp -r ../checkouts/Adwaita/.git/* ./Adwaita-39caa65a
mkdir ./LevenshteinTransformations-222f7af2
cp -r ../checkouts/LevenshteinTransformations/.git/* ./LevenshteinTransformations-222f7af2
mkdir ./Localized-2b72df47
cp -r ../checkouts/Localized/.git/* ./Localized-2b72df47
mkdir ./swift-macro-toolkit-40d6b9db
cp -r ../checkouts/swift-macro-toolkit/.git/* ./swift-macro-toolkit-40d6b9db
mkdir ./swift-syntax-e33d5ec5
cp -r ../checkouts/swift-syntax/.git/* ./swift-syntax-e33d5ec5
mkdir ./Yams-81702b8f
cp -r ../checkouts/Yams/.git/* ./Yams-81702b8f
