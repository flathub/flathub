# EasyEffects
[EasyEffects](https://github.com/wwmm/easyeffects) is the successor to PulseEffects. Only PipeWire is natively supported, if you use PulseAudio install [PulseEffects](https://flathub.org/apps/details/com.github.wwmm.pulseeffects).

To build locally (when PR is still opened):
1. `git clone --recurse-submodules -j8 https://github.com/vchernin/flathub.git --branch com.github.wwmm.easyeffects --single-branch`
2. `cd flathub`
3. `flatpak-builder build-dir --user --install com.github.wwmm.easyeffects.yml`

Debugging (by entering the Flatpak sandbox):
1. `flatpak run --command=sh --devel com.github.wwmm.easyeffects` 
2. `G_MESSAGES_DEBUG=easyeffects easyeffects` and/or
3. `GTK_DEBUG=all easyeffects`