# 2dTaskBoard

Task management productivity app with markdown support.

With **2dTaskBoard** you can track the progress of your daily tasks. Key features:

- **2D Board** - move tasks between rows and columns.
- **File system based** - all your data are files which can be then synced using cloud providers or git
- **Markdown support** - rich text formatting
- **Mermaid** support - flowcharts rendering
- **Attachments** - attach files of any kind to your tasks
- **Obsidian** compatibility - open tasks in Obsidian and link to existing notes
- **Full keyboard support** - every part of this app is accessible just by pressing keyboard shortcuts

<img src="https://github.com/piotrek-k/2dTaskBoard/blob/90f2f90f31fcf96119082573044eea8fa0fb57b6/screenshots/board_view.png?raw=true" alt="Board view" style="max-width: 300px; height: auto;">
<img src="https://github.com/piotrek-k/2dTaskBoard/blob/90f2f90f31fcf96119082573044eea8fa0fb57b6/screenshots/task_view.png?raw=true" alt="Task view" style="max-width: 300px; height: auto;">


## Useful commands

Build:
```
flatpak-builder build io.github.piotrek_k._2dTaskBoard.yml --install-deps-from=flathub --force-clean --user --install
```

Run:
```
flatpak run io.github.piotrek_k._2dTaskBoard
```

To build `generated-sources.json` use `flatpak-node-generator`.

```
flatpak-node-generator npm package-lock.json --no-requests-cache
```

Manifest validation:
```
flatpak run --command=flatpak-builder-lint org.flatpak.Builder appstream io.github.piotrek_k._2dTaskBoard.metainfo.xml
```