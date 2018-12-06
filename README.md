Diffoscope
==========

In-depth comparison of files, archives, and directories.

* [Upstream Website](https://diffoscope.org/)

Flatpak Maintenance
-------------------

In theory, update `python3-diffoscope.json` with:

```bash
flatpak-pip-generator --output python3-diffoscope.json \
  'diffoscope[distro_detection,cmdline,comparators]'
```

But unfortunately some of the optional packages pulled in by `diffoscope[comparators]` are not on PyPI (namely `guestfs`, `rpm-python` and `tlsh`). So, expand that list manually, removing those problematic ones:

```bash
flatpak-pip-generator --output python3-diffoscope.json \
  'diffoscope[distro_detection,cmdline]' \
  binwalk defusedxml jsondiff python-debian pypdf2 pyxattr
```
