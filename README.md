# Installation

## Installing:
### Install pre-requisite
```
$ flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
$ flatpak install flathub org.freedesktop.Platform//20.08 org.freedesktop.Sdk//20.08 --system
```
### Build + Install / Update
```
$ flatpak-builder builddir com.github.nihui.waifu2x-ncnn-vulkan.yaml --force-clean --install --user
```

## Running:
See <https://github.com/nihui/waifu2x-ncnn-vulkan#usage> for details.  
With flatpak you would do this:
```
flatpak run com.github.nihui.waifu2x-ncnn-vulkan -i [input.png] -o [output.png] -n [noise-level] -s [scale] -t [tile size] -m [model dir]
```
For example (using default model and denoise disabled):
```
$ flatpak run com.github.nihui.waifu2x-ncnn-vulkan -i ~/Pictures/foo.jpg -o ~/Pictures/bar.png -n -1 -s 2 -t 400
```

### Run with Mesa ACO
More info here (in Japanese): <https://blog.coelacanth-dream.com/posts/2020/04/26/waifu2x-ncnn-vulkan-speedup-2x-aco/>  
For a single run, pass `--env=RADV_PERFTEST=aco` option to `flatpak run`.  
Example:
```
flatpak run --env=RADV_PERFTEST=aco com.github.nihui.waifu2x-ncnn-vulkan -i ~/Pictures/foo.jpg -o ~/Pictures/bar.png -n -1 -s 2 -t 400
```
To make the option permanent:
```
flatpak override --env=RADV_PERFTEST=aco com.github.nihui.waifu2x-ncnn-vulkan
```

## Notes:
By default, this flatpak only allows reading and saving to **XDG-PICTURES** directory.  
If you want to save/read from other locations:
```
flatpak override com.github.nihui.waifu2x-ncnn-vulkan --filesystem=host
```
