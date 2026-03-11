# Opera

This repo hosts the flatpak version of [Opera GX Browser](https://www.opera.com/gx/)

## Options

You can change the browser language by adding a startup parameter `lang`, full example:

```
/usr/bin/flatpak run --branch=stable --arch=x86_64 --command=opera --file-forwarding com.opera.Opera @@U --lang=cs %U @@
```

## Video playback

You don't need to change anything to get video working, this flatpak automatically includes [FFmpeg prebuilt codecs](https://github.com/Ld-Hagen/nwjs-ffmpeg-prebuilt/) in the location where Opera GX expects them.
