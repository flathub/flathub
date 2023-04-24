# Regarding flatpak-pip-generator

I failed to get it working with the default script, something always went wrong,
so I tweaked it to prefer the platform-independent .whl files over other archives,
and that finally allowed me to build a flatpak (after a day of suffering) :) 


## Notes:

Build commands:
```bash
python flatpak-pip-generator-fix --runtime='org.freedesktop.Sdk//22.08' --yaml --output pypi-dependencies --requirements-file='requirements.txt'
```

```bash
flatpak-builder --repo=myrepo --force-clean build-dir com.github.voxelcubes.deepqt.yaml
```

```bash
flatpak build-bundle myrepo deepqt.flatpak com.github.voxelcubes.deepqt 
```

```bash
flatpak install --user deepqt.flatpak   
```