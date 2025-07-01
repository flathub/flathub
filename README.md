# Candle Genie Basic

Candle Genie Basic is the absolute bare-bones, demo version of Candle Genie.
This edition is intentionally limited—think of it as a free "trial" or showcase for the real thing.

No save/load features

Limited to 2-candlestick pattern creation

Only basic, hard-coded candlestick patterns included

Manual workflow: If you want to use any data, you must transfer everything by hand—there is no export, automation, or convenience features.

This version exists to prove the UI and show basic pattern-building functionality. That’s it.




The full-featured "Pro" version—offering a full list of pre-programmed single candles, 

up to 5-candlestick pattern creation, open and save unlimited pattern libraries, unlimited patterns, 

advanced editing, and time-saving automation—is just $1 at: https://candlegenie.systeme.io/

## Build and Run (Flatpak)

```sh
flatpak-builder --force-clean build-dir io.aventro.CandleGenieBasic.yml
flatpak build-export repo build-dir
flatpak remote-add --user --no-gpg-verify candlegenie-local repo || true
flatpak install --user --reinstall candlegenie-local io.aventro.CandleGenieBasic
flatpak run io.aventro.CandleGenieBasic
