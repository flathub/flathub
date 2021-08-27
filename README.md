# Konsole (org.kde.konsole)

[Homepage] | [KDE Apps] | [User documentation] | [Sources]

## Reporting bugs

**Before filling a bug, please test if the bug is reproduceable with the
classic non-Flatpak version of this application.**

- If the bug is only reproducible with the **Flatpak version** of this
  application, please file an [issue here][issue].

- If the bug is also reproducible with the **non-Flatpak version** of this
  application, please take a look at the [currently opened bugs][bugs] and if
  it has not been reported yet, file a bug at
  [bugs.kde.org].

**If you have any doubt, please file an [issue here][issue].**

## Permissions rationale

This application requests the following restricted permissions:

- Required for accessibility: `--talk-name=org.a11y.Bus`
- Required to run commands on the host: `--talk=TODO`

[Homepage]: https://konsole.kde.org/
[KDE Apps]: https://apps.kde.org/konsole/
[User documentation]: https://userbase.kde.org/Konsole
[Sources]: https://invent.kde.org/utilities/konsole
[issue]: https://github.com/flathub/org.kde.konsole/issues/new
[bugs]: https://bugs.kde.org/buglist.cgi?bug_status=UNCONFIRMED&bug_status=CONFIRMED&bug_status=ASSIGNED&bug_status=REOPENED&product=konsole&query_format=advanced
[bugs.kde.org]: https://bugs.kde.org/enter_bug.cgi?format=guided&product=konsole
