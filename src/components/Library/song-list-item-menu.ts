import type { ContextMenuItem } from "./ContextMenu";

type TranslateFn = (
  key: string,
  options?: Record<string, string | number>,
) => string;

interface BuildSongListContextMenuItemsArgs {
  t: TranslateFn;
  isMultiSelected: boolean;
  selectedCount: number;
  selectedSongIds: string[];
  selectedHasSeparableSongs: boolean;
  selectedCanToggleInstrumentalSongs: boolean;
  selectedInstrumentalState: "checked" | "mixed" | "unchecked";
  supportsEmbeddedLyrics: boolean;
  queueAllSelected: () => void;
  separateAllSelected: () => void;
  toggleSelectedInstrumental: () => void;
  extractSelectedEmbeddedCoverArt: () => void;
  deleteSelected: () => void;
  playNow: () => void;
  playNext: () => void;
  addToQueue: () => void;
  extractEmbeddedCoverArt: () => void;
  extractEmbeddedLyrics: () => void;
  fetchLyricsOnline: () => void;
  editInfo: () => void;
  openProperties: () => void;
  deleteSong: () => void;
}

export function buildSongListContextMenuItems({
  t,
  isMultiSelected,
  selectedCount,
  selectedSongIds,
  selectedHasSeparableSongs,
  selectedCanToggleInstrumentalSongs,
  selectedInstrumentalState,
  supportsEmbeddedLyrics,
  queueAllSelected,
  separateAllSelected,
  toggleSelectedInstrumental,
  extractSelectedEmbeddedCoverArt,
  deleteSelected,
  playNow,
  playNext,
  addToQueue,
  extractEmbeddedCoverArt,
  extractEmbeddedLyrics,
  fetchLyricsOnline,
  editInfo,
  openProperties,
  deleteSong,
}: BuildSongListContextMenuItemsArgs): ContextMenuItem[] {
  if (isMultiSelected) {
    const instrumentalIndicator: ContextMenuItem["indicator"] =
      selectedInstrumentalState === "checked"
        ? "checked"
        : selectedInstrumentalState === "mixed"
          ? "mixed"
          : null;

    return [
      {
        label: t("library.queueAllSelected", {
          count: selectedCount || selectedSongIds.length,
        }),
        onClick: queueAllSelected,
      },
      ...(selectedCanToggleInstrumentalSongs
        ? [
            {
              label: t("library.markInstrumentalSelected", {
                count: selectedCount || selectedSongIds.length,
              }),
              onClick: toggleSelectedInstrumental,
              indicator: instrumentalIndicator,
            },
          ]
        : []),
      ...(selectedHasSeparableSongs
        ? [
            {
              label: t("library.separateAllSelected", {
                count: selectedCount || selectedSongIds.length,
              }),
              onClick: separateAllSelected,
            },
          ]
        : []),
      {
        label: t("library.extractEmbeddedCoverArtSelected", {
          count: selectedCount || selectedSongIds.length,
        }),
        onClick: extractSelectedEmbeddedCoverArt,
      },
      {
        label: t("library.deleteSelected", {
          count: selectedCount || selectedSongIds.length,
        }),
        onClick: deleteSelected,
      },
    ];
  }

  return [
    {
      label: t("library.playNow"),
      onClick: playNow,
    },
    {
      label: t("library.playNext"),
      onClick: playNext,
    },
    {
      label: t("library.addToQueue"),
      onClick: addToQueue,
    },
    {
      label: t("library.extractEmbeddedCoverArt"),
      onClick: extractEmbeddedCoverArt,
    },
    ...(supportsEmbeddedLyrics
      ? [
          {
            label: t("library.extractEmbeddedLyrics"),
            onClick: extractEmbeddedLyrics,
          },
        ]
      : []),
    {
      label: t("library.fetchLyricsOnline"),
      onClick: fetchLyricsOnline,
    },
    {
      label: t("library.editInfo"),
      onClick: editInfo,
    },
    {
      label: t("library.properties"),
      onClick: openProperties,
    },
    {
      label: t("library.delete"),
      onClick: deleteSong,
    },
  ];
}
