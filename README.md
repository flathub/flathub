Run in project root directory:

```shell
flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install --user -y flathub org.flatpak.Builder
python -m pip install '.[gui,basicvsrpp,packaging-flatpak]'
pip-compile --extra gui,basicvsrpp -o flatpak/requirements.txt setup.py
req2flatpak --requirements-file flatpak/requirements.txt --target-platforms 312-x86_64 > flatpak/lada-pip-dependencies.json.tmp
jq '.sources |= [.[] |select(.url == "https://files.pythonhosted.org/packages/e9/a2/57a733e7e84985a8a0e3101dfb8170fc9db92435c16afad253069ae3f9df/mmcv-2.2.0.tar.gz") += {url: "https://download.openmmlab.com/mmcv/dist/cu121/torch2.4.0/mmcv-2.2.0-cp312-cp312-manylinux1_x86_64.whl", sha256: "0c5fa8f302f99f6b64f11c767ceb504def84ef44849fc83378a99906463ac68e"}]' flatpak/lada-pip-dependencies.json.tmp > flatpak/lada-pip-dependencies.json
rm flatpak/lada-pip-dependencies.json.tmp
flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak run org.flatpak.Builder --force-clean --sandbox --user --state-dir=flatpak/state --repo=flatpak/repo --install --install-deps-from=flathub --mirror-screenshots-url=https://dl.flathub.org/media/ flatpak/build  flatpak/io.github.ladaapp.lada.yaml
```

