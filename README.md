# Installation

## Installing:
```
flatpak-builder builddir com.github.nihui.waifu2x-ncnn-vulkan.yaml --force-clean --install --user
```

## Running:
See https://github.com/nihui/waifu2x-ncnn-vulkan#usage for details.  
With flatpak you would do this:
```
flatpak run com.github.nihui.waifu2x-ncnn-vulkan [input image] [output png] [noise=-1/0/1/2/3] [scale=1/2] [blocksize=400]
```
For example:
```
flatpak run com.github.nihui.waifu2x-ncnn-vulkan ~/Pictures/foo.jpg ~/Pictures/bar.png -1 2 400
```

## Notes:
By default, this flatpak only allows reading and saving to **XDG-PICTURES** directory.  
If you want to save/read from other locations:
```
flatpak override com.github.nihui.waifu2x-ncnn-vulkan --filesystem=host
```
