
# Translations

This folder contains translation files in `.po` (Portable Object) format. Other translations (such as the app description) are located in the [metainfo file](../data/io.github.noobping.listenmoe.metainfo.xml). If you spot a typo, unclear wording, or have a better translation, contributions are welcome.

## What to edit

Each `.po` file contains entries like:

```text
msgid "Original text"
msgstr "Translated text"
```

Only edit the msgstr line. Do not change msgid.

## Edit on GitHub

1. Open a `.po` file in this folder on GitHub
2. Click the Edit (pencil) button
3. GitHub will ask you to fork the repository, accept it
4. Improve the `msgstr` text
5. Propose the changes and create a Pull Request

## Alternative

If you do not want to edit files on GitHub, you can open an [issue](https://github.com/noobping/listenmoe/issues) instead. In the issue, include the original text (`msgid`), your suggested translation (`msgstr`), and the language file name (for example `ja.po`).

## Guidelines

Keep the meaning of the original text the same. Do not add or remove information unless necessary for the language. Placeholders such as `%s`, `{name}`, or `%(count)d` must remain exactly the same. They may be moved if required by grammar, but must not be changed or removed. Do not edit `msgid`, `comments`, or `file headers`. Only modify the `msgstr` values.
