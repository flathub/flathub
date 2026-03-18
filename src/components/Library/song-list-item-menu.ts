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
  supportsEmbeddedLyrics: boolean;
  queueAllSelected: () => void;
  separateAllSelected: () => void;
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
  supportsEmbeddedLyrics,
  queueAllSelected,
  separateAllSelected,
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
    return [
      {
        label: t("library.queueAllSelected", {
          count: selectedCount || selectedSongIds.length,
        }),
        onClick: queueAllSelected,
      },
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
