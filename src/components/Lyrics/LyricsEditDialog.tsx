import { useState, useEffect } from "react";
import { useLyricsStore } from "@/stores/lyrics-store";

interface LyricsEditDialogProps {
  open: boolean;
  onClose: () => void;
  songId: string;
  existingLyrics?: string;
}

export function LyricsEditDialog({
  open,
  onClose,
  songId,
  existingLyrics,
}: LyricsEditDialogProps) {
  const [text, setText] = useState(existingLyrics ?? "");

  useEffect(() => {
    if (open) {
      setText(existingLyrics ?? "");
    }
  }, [open, existingLyrics]);

  if (!open) return null;

  const isLrc = /\[\d{2}:\d{2}/.test(text);

  const handleSave = async () => {
    await useLyricsStore.getState().saveManualLyrics(songId, text);
    onClose();
  };

  return (
    <div
      className="fixed inset-0 z-50 flex items-center justify-center bg-black/60"
      onClick={(e) => {
        if (e.target === e.currentTarget) onClose();
      }}
    >
      <div className="flex w-full max-w-lg flex-col gap-4 rounded-xl border border-[#3A3A3C] bg-[#1C1C1E] p-6 shadow-2xl">
        <h2 className="text-[15px] font-semibold text-white">Edit Lyrics</h2>

        <textarea
          value={text}
          onChange={(e) => setText(e.target.value)}
          placeholder="Paste or type lyrics here..."
          className="h-64 w-full resize-y rounded-md border border-[#3A3A3C] bg-[#2C2C2E] px-3 py-2 text-[13px] text-white placeholder-[#636366] focus:border-[#48484A] focus:outline-none"
          spellCheck={false}
        />

        <p className="text-[11px] text-[#8E8E93]">
          {text.trim().length > 0
            ? isLrc
              ? "Detected: LRC format"
              : "Detected: Plain text"
            : "Supports LRC timed format or plain text"}
        </p>

        <div className="flex justify-end gap-2">
          <button
            onClick={onClose}
            className="rounded-md px-4 py-1.5 text-[13px] text-[#EBEBF5] transition-colors hover:bg-[#3A3A3C]"
          >
            Cancel
          </button>
          <button
            onClick={handleSave}
            disabled={text.trim().length === 0}
            className="rounded-md bg-[#3A3A3C] px-4 py-1.5 text-[13px] text-[#EBEBF5] transition-colors hover:bg-[#48484A] disabled:opacity-40 disabled:hover:bg-[#3A3A3C]"
          >
            Save
          </button>
        </div>
      </div>
    </div>
  );
}
