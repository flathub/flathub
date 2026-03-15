import { type ReactNode } from "react";
import { open } from "@tauri-apps/plugin-dialog";
import { audioDir } from "@tauri-apps/api/path";
import { useLibraryStore } from "@/stores/library-store";

interface ImportButtonProps {
  children: ReactNode;
}

const AUDIO_EXTENSIONS = [
  "mp3",
  "flac",
  "wav",
  "ogg",
  "m4a",
  "aac",
  "wma",
  "lrc",
];

export function ImportButton({ children }: ImportButtonProps) {
  const importFiles = useLibraryStore((s) => s.importFiles);

  const handleClick = async () => {
    let defaultPath: string | undefined;
    try {
      defaultPath = await audioDir();
    } catch {
      // audioDir may not be available on all platforms; fall through
    }

    const selected = await open({
      multiple: true,
      defaultPath,
      filters: [
        {
          name: "Audio & Lyrics",
          extensions: AUDIO_EXTENSIONS,
        },
      ],
    });

    if (selected && selected.length > 0) {
      importFiles(selected);
    }
  };

  return <button onClick={handleClick}>{children}</button>;
}
