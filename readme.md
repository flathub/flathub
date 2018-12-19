## Instruction 

flatpak-builder --repo=qtodo-repo qtodo python3-qtodotxt.json

flatpak --user remote-add --no-gpg-verify --if-not-exists qtodo-repo qtodo-repo

flatpak install qtodo-repo org.qtodotxt.qtodotxt


