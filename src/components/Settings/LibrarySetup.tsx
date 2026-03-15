import { useState } from "react";
import { open } from "@tauri-apps/plugin-dialog";
import { FolderOpen, Plus, Music } from "lucide-react";
import * as api from "@/lib/tauri";

interface LibrarySetupProps {
  onComplete: () => void;
}

export function LibrarySetup({ onComplete }: LibrarySetupProps) {
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);

  const handleCreate = async () => {
    const selected = await open({
      directory: true,
      title: "Choose a location for your Karaoke Library",
    });

    if (!selected) return;

    const libraryDir = `${selected}/OpenKara`;
    setLoading(true);
    setError(null);
    try {
      await api.createLibrary(libraryDir);
      onComplete();
    } catch (err: unknown) {
      const message =
        err instanceof Error ? err.message : String(err);
      setError(message);
    } finally {
      setLoading(false);
    }
  };

  const handleOpen = async () => {
    const selected = await open({
      directory: true,
      title: "Open an existing Karaoke Library",
    });

    if (!selected) return;

    setLoading(true);
    setError(null);
    try {
      await api.openLibrary(selected);
      onComplete();
    } catch (err: unknown) {
      const message =
        err instanceof Error ? err.message : String(err);
      setError(message);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="flex h-screen w-full items-center justify-center bg-[var(--color-surface)]">
      <div className="mx-auto max-w-md space-y-8 px-6 text-center">
        <div className="flex flex-col items-center gap-4">
          <div className="flex h-16 w-16 items-center justify-center rounded-2xl bg-[var(--color-accent)]/15">
            <Music size={32} className="text-[var(--color-accent)]" />
          </div>
          <h1 className="text-2xl font-bold text-white">Welcome to OpenKara</h1>
          <p className="text-[14px] leading-relaxed text-[var(--color-text-dim)]">
            Set up a Karaoke Library to store your imported songs and separated
            tracks. The library is portable — you can place it on a NAS or
            external drive and open it from any device.
          </p>
        </div>

        <div className="space-y-3">
          <button
            onClick={handleCreate}
            disabled={loading}
            className="flex w-full items-center gap-3 rounded-lg border border-[var(--color-border-light)] bg-[var(--color-sidebar)] px-5 py-4 text-left transition-colors hover:bg-[var(--color-hover)] disabled:opacity-50"
          >
            <Plus size={20} className="shrink-0 text-[var(--color-accent)]" />
            <div>
              <div className="text-[14px] font-medium text-white">
                Create New Library
              </div>
              <div className="text-[12px] text-[var(--color-text-dim)]">
                Choose a folder to create a new karaoke library
              </div>
            </div>
          </button>

          <button
            onClick={handleOpen}
            disabled={loading}
            className="flex w-full items-center gap-3 rounded-lg border border-[var(--color-border-light)] bg-[var(--color-sidebar)] px-5 py-4 text-left transition-colors hover:bg-[var(--color-hover)] disabled:opacity-50"
          >
            <FolderOpen
              size={20}
              className="shrink-0 text-[var(--color-text-dim)]"
            />
            <div>
              <div className="text-[14px] font-medium text-white">
                Open Existing Library
              </div>
              <div className="text-[12px] text-[var(--color-text-dim)]">
                Select a folder containing an OpenKara library
              </div>
            </div>
          </button>
        </div>

        {error && (
          <p className="text-[13px] text-red-400">{error}</p>
        )}

        {loading && (
          <p className="text-[13px] text-[var(--color-text-dim)]">
            Setting up library…
          </p>
        )}
      </div>
    </div>
  );
}
