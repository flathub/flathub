# EasyEffects

### Audio effects for PipeWire applications.

[EasyEffects](https://github.com/wwmm/easyeffects) is the successor to PulseEffects. Only PipeWire is natively supported, if you use PulseAudio install [PulseEffects](https://flathub.org/apps/details/com.github.wwmm.pulseeffects).

If EasyEffects does not start, ensure you have PipeWire `0.3.31` or newer on your host using `pipewire --version`.

For advanced users or for bug reporting:

To build locally :
1. `git clone --recurse-submodules https://github.com/flathub/com.github.wwmm.easyeffects.git`
2. `cd com.github.wwmm.easyeffects`
3. `flatpak-builder build-dir --user --install com.github.wwmm.easyeffects.yml`

Debugging (by entering the Flatpak sandbox):
1. `flatpak run --command=sh --devel com.github.wwmm.easyeffects` 

2. Enter one of the following:

- For EasyEffects logs:  
`G_MESSAGES_DEBUG=easyeffects easyeffects`

- For GTK's debugger:  
`GTK_DEBUG=all easyeffects`