#!/usr/bin/env bash
mkdir .build/repositories
cd .build/repositories
mkdir ./Adwaita-efb31624
cp -r ../checkouts/Adwaita/.git/* ./Adwaita-efb31624
mkdir ./FuzzyFind-c2e7bad1
cp -r ../checkouts/FuzzyFind/.git/* ./FuzzyFind-c2e7bad1
mkdir ./LevenshteinTransformations-2e99b7c3
cp -r ../checkouts/LevenshteinTransformations/.git/* ./LevenshteinTransformations-2e99b7c3
mkdir ./Localized-f70fe787
cp -r ../checkouts/Localized/.git/* ./Localized-f70fe787
mkdir ./Yams-fd9f2519
cp -r ../checkouts/Yams/.git/* ./Yams-fd9f2519