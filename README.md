# Python DHCP Server Flatpak Specification

This is the flatpak repository for
[Python DHCP Server][app]

[app]: https://github.com/niccokunzmann/python_dhcp_server/

## New Releases

1. Create a [new release](https://github.com/niccokunzmann/python_dhcp_server/releases).
2. In `io.github.niccokunzmann.python_dhcp_server.yml`:
    - Increase the tag
    - Update the hash
3. Edit `app/io.github.niccokunzmann.python_dhcp_server.xml` to contain
   the latest release information at the bottom.
4. Create a PR for a new release.
   ```
   git checkout -b release
   git commit -am "new release"
   git push origin release
   ```
