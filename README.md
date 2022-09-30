# GPU Screen Recorder
This is a screen recorder that has minimal impact on system performance by recording a window using the GPU only, similar to shadowplay on windows. This is the fastest screen recording tool for Linux. It's currently limited to nvidia GPUs.

# Info about NvFBC
Recording monitors requires a gpu with NvFBC support (note: this is not required when recording a single window!). Normally only tesla and quadro gpus support this, but by using [nvidia-patch](https://github.com/keylase/nvidia-patch) and running `patch.sh` and `patch-fbc.sh` with the `-f` option you can patch your flatpak nvidia driver to support NvFBC. This should be done at your own risk.
