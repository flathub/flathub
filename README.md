# ungoogled-chromium flatpak

<!-- ## How do I install Widevine CDM?

1. Go to https://github.com/mozilla/gecko-dev/blob/master/toolkit/content/gmp-sources/widevinecdm.json
2. Download for your architecture (ex. `Linux_x86_64-gcc3` if you're on x86_64). Get the URL in `fileURL` and download and extract the zip file.
3. Now do `mv libwidevinecdm.so ~/.var/app/com.github.Eloston.UngoogledChromium/config/chromium/WidevineCdm`.
-->
## How do I fix the spell checker?

1. Go to https://chromium.googlesource.com/chromium/deps/hunspell_dictionaries/+/master
2. Find a bdic you want, click on it. You will see a mostly empty page aside from “X-byte binary file”
3. On the bottom right corner, click “txt”. For en-US-9-0.bdic, you will get a link https://chromium.googlesource.com/chromium/deps/hunspell_dictionaries/+/master/en-US-9-0.bdic?format=TEXT
4. This is a base64-encoded file that needs to be decoded.
5. Now, simply run `base64 -d en-US-9-0.bdic > ~/.var/app/com.github.Eloston.UngoogledChromium/config/chromium/Dictionaries/en-US-9-0.bdic` (assuming you want the dictionary to be in the default profile)
6. Toggle spell check in `chrome://settings/languages`, or restart the browser for the dictionaries to take effect.

## For other problems please visit https://ungoogled-software.github.io/ungoogled-chromium-wiki/faq
