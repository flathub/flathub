# dev.overlayed.Overlayed 

Here are my notes that I've been keeping in order to test this build locally. Sadly since I'm on tauri v1 it means we have to compile webkitgtk-4.0 from source which takes a LOONG time.

It's not really feasible to allow me to test via a github action so I'm using my gaming laptop+WSL.

Build/run the app via [WSLG](https://github.com/microsoft/wslg).
```
make && make test
```

### WSL Gotcha

When first trying to compile it I ran out of memory so I had to add this to my `%USERPROFILE%/.wslconfig`

```
# Settings apply across all Linux distros running on WSL 2
[wsl2]

# Limits VM memory to use no more than 4 GB, this can be set as whole numbers using GB or MB
memory=20GB 

# Sets the VM to use two virtual processors
processors=8

# Sets amount of swap storage space to 8GB, default is 25% of available RAM
swap=8GB
```

It also still took me around an hour to compile for the first time 😂. Then after that with the `--install` flag it takes around a couple of mins.

