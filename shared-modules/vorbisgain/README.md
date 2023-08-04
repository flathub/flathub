Vorbisgain comes in two version;
- The main source: https://sjeng.org/ftp/vorbis/
- The Debian version: http://deb.debian.org/debian/pool/main/v/vorbisgain/

The catch is... Debian has 10 patches:
- 0001-temp_files.patch
- 0002-errno.patch
- 0003-manpage.patch
- 0004-vorbisgain_mtime.patch
- 0005-double_fclose.patch
- 0006-manpage_hyphens.patch
- 0007-recursively_spelling.patch
- 0008-manpage_recursion_mistake.patch
- 0009-hardening.patch
- 0010-fclose.patch

Of particular concern are 0005, 0009 and 0010 which might have implications for memory-leaks and string inputs.

## Other points of concern

https://pkgs.org/download/vorbisgain

Lots of distributions don't include these patches as well, but I prefer to stick with the tried and tested package from Debian.
