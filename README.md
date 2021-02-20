# SQurreL SQL - Flatpak

SQuirreL SQL Client is a graphical Java program that will allow you to view the
structure of a JDBC compliant database, browse the data in tables, issue SQL
commands etc.

The sandbox permissions are pretty restrictive at this point. The only
filesystems permissions are for a persistent (but sandboxed) ~/.squirrel-sql
directory.

## Installation

Local installation via flatpak-builder:

```bash
flatpak-builder --user --force-clean --install build org.squirrelsql.squirrelsql.json 
```

This flatpak only includes the mariadb java connector, and it has to be
configured by adding the class path
`/app/mariadb-java-connector/mariadb-java-client.jar` to the driver.

## License

Flatpak support & metadata files in this repository: CC0-1.0

SQuirreL SQL: LGPL-2.1-or-later
