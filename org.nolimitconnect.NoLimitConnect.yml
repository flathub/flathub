id: org.nolimitconnect.NoLimitConnect

runtime: org.kde.Platform
runtime-version: '6.10'
sdk: org.kde.Sdk

command: nolimitconnect

finish-args:
  # Display
  - --socket=wayland
  - --socket=fallback-x11

  # Audio / Video
  - --socket=pulseaudio
  - --device=dri

  # Network & IPC
  - --share=network
  - --share=ipc

cleanup:
  - '*.a'
  - '*.o'

modules:
  - name: nolimitconnect
    buildsystem: cmake-ninja
    config-opts:
      - -DCMAKE_BUILD_TYPE=Release
      - -DFLATPAKBUILD=ON

    sources:
      # v1.1.1 release source for Flathub submission
      - type: archive
        url: https://github.com/nolimitconnect/NoLimitConnect/archive/88d20b46b6051c094f6219a0a2ae91b7d0e18eb6.tar.gz
        sha256: a96c4ee3d9ee01f61ee7d9960de05a701decf07c0cc189c798ca45540ae2ae38





