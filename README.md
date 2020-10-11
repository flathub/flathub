# KTorrent (org.kde.ktorrent)

[Homepage](https://kde.org/applications/ktorrent) |
[User documentation](https://docs.kde.org/stable5/en/extragear-network/ktorrent/index.html) |
[Sources](https://invent.kde.org/network/ktorrent)

## Reporting bugs

**Before filling a bug, please test if the bug is reproduceable with the
classic non-Flatpak version of this application.**

- If the bug is only reproducible with the **Flatpak version** of this
  application, please file an [Issue here][issue].

- If the bug is also reproducible with the **non-Flatpak version** of this
  application, please take a look at the [currently opened bugs][bugs] and if
  it has not been reported yet, file a bug at
  [bugs.kde.org](https://bugs.kde.org).

**If you have any doubt, please file an [Issue here][issue].**

## Permissions rationale

This application requests the following restricted permissions:

- Required to write torrents to a user chosen path: `--filesystem=host`

[issue]: https://github.com/flathub/org.kde.ktorrent/issues/new
[bugs]: https://bugs.kde.org/buglist.cgi?bug_status=UNCONFIRMED&bug_status=CONFIRMED&bug_status=ASSIGNED&bug_status=REOPENED&product=ktorrent&query_format=advanced
