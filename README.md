# SQurreL SQL - Flatpak

SQuirreL SQL Client is a graphical Java program that will allow you to view the
structure of a JDBC compliant database, browse the data in tables, issue SQL
commands etc.

The sandbox permissions are pretty restrictive at this point, and the application
has no access outside of the sandbox.

## Installation

Local installation via flatpak-builder:

```bash
flatpak-builder --user --force-clean --install build org.squirrelsql.squirrelsql.json
```

This flatpak includes the JDBC drivers for PostgreSQL, MySQL, SQLite, and MariaDB.

## License

Flatpak support & metadata files in this repository: CC0-1.0

SQuirreL SQL: LGPL-2.1-or-later
