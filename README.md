# Installation

## Installing:
### Install pre-requisite
```
$ flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
$ flatpak install flathub org.freedesktop.Platform//18.08 org.freedesktop.Sdk//18.08 --system
```
### Build + Install
```
$ flatpak-builder builddir com.github.nihui.waifu2x-ncnn-vulkan.yaml --force-clean --install --user
```

## Running:
See https://github.com/nihui/waifu2x-ncnn-vulkan#usage for details.  
With flatpak you would do this:
```
flatpak run com.github.nihui.waifu2x-ncnn-vulkan -i [input.png] -o [output.png] -n [noise-level] -s [scale] -t [tile size] -m [model dir]
```
For example (using default model and denoise disabled):
```
$ flatpak run com.github.nihui.waifu2x-ncnn-vulkan -i ~/Pictures/foo.jpg -o ~/Pictures/bar.png -n -1 -s 2 -t 400
```


## Notes:
By default, this flatpak only allows reading and saving to **XDG-PICTURES** directory.  
If you want to save/read from other locations:
```
flatpak override com.github.nihui.waifu2x-ncnn-vulkan --filesystem=host
```
