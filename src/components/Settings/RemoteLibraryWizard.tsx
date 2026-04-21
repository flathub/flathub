import { useState } from "react";
import { useTranslation } from "react-i18next";
import { Cloud, FolderOpen, Globe, X } from "lucide-react";
import * as api from "@/lib/tauri";
import type { RegisteredLibrary, RemoteLibraryProvider } from "@/types/ipc";
import { useSettingsOverlay } from "./SettingsOverlay.context";

type RemoteSetupMode = "open_remote" | "mirror_active_local";

const remoteProviderDisplayNames: Record<RemoteLibraryProvider, string> = {
  google_drive: "Google Drive Library",
  dropbox: "Dropbox Library",
  webdav: "WebDAV Library",
};

export function RemoteLibraryWizard({ onClose }: { onClose: () => void }) {
  const { t } = useTranslation();
  const { state, meta, actions } = useSettingsOverlay();
  const [mode, setMode] = useState<RemoteSetupMode>("open_remote");
  const [provider, setProvider] =
    useState<RemoteLibraryProvider>("google_drive");
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [message, setMessage] = useState<string | null>(null);
  const [authorizationUrl, setAuthorizationUrl] = useState<string | null>(null);
  const [displayName, setDisplayName] = useState(
    remoteProviderDisplayNames.google_drive,
  );
  const [serverUrl, setServerUrl] = useState("");
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [rootPath, setRootPath] = useState("/OpenKara");

  const activeLibrary = state.libraries.find(
    (library) => library.id === state.activeLibraryId,
  );
  const activeLocalLibrary =
    activeLibrary?.kind === "local" ? activeLibrary : null;
  const canMirrorActiveLocal = activeLocalLibrary !== null;

  const resetProviderState = (nextProvider: RemoteLibraryProvider) => {
    setProvider(nextProvider);
    setDisplayName(remoteProviderDisplayNames[nextProvider]);
    setError(null);
    setMessage(null);
    setAuthorizationUrl(null);
  };

  const completeBrowserAuth = async (
    sessionId: string,
    nextProvider: RemoteLibraryProvider,
  ) => {
    const deadline = Date.now() + 120_000;
    while (Date.now() < deadline) {
      await new Promise((resolve) => window.setTimeout(resolve, 1000));
      const status = await api.pollRemoteAuth(sessionId);
      if (status.state === "ready") {
        return;
      }
      if (status.state === "failed") {
        throw new Error(
          status.error?.message ?? "Remote sign-in failed unexpectedly.",
        );
      }
    }

    throw new Error(
      nextProvider === "google_drive"
        ? "Google sign-in timed out before OpenKara received the callback."
        : nextProvider === "dropbox"
          ? "Dropbox sign-in timed out before OpenKara received the callback."
          : "Remote sign-in timed out.",
    );
  };

  const connect = async () => {
    if (provider === "webdav") {
      if (!serverUrl.trim()) {
        setError("Enter the WebDAV server URL first.");
        return;
      }
      if (!username.trim()) {
        setError("Enter the WebDAV username first.");
        return;
      }
      if (!password.trim()) {
        setError("Enter the WebDAV password first.");
        return;
      }
    }

    if (mode === "mirror_active_local" && !activeLocalLibrary) {
      setError("Switch to the local library you want to mirror first.");
      return;
    }

    setLoading(true);
    setError(null);
    setMessage(null);
    setAuthorizationUrl(null);

    try {
      const start = await api.beginRemoteAuth(
        provider,
        provider === "webdav"
          ? {
              type: "webdav",
              server_url: serverUrl,
              username,
              password,
              root_path: rootPath.trim() || null,
            }
          : null,
      );

      if (start.authorization_url) {
        setAuthorizationUrl(start.authorization_url);
        globalThis.open?.(
          start.authorization_url,
          "_blank",
          "noopener,noreferrer",
        );
        await completeBrowserAuth(start.session_id, provider);
      }

      const candidate = await api.createRemoteLibrary(
        start.session_id,
        displayName.trim() || remoteProviderDisplayNames[provider],
      );
      const registry = await api.registerRemoteLibrary(
        start.session_id,
        candidate.remote_root_locator,
        displayName.trim() || candidate.display_name,
      );
      const remoteLibraryId = registry.active_library_id;

      if (!remoteLibraryId) {
        throw new Error("The new remote library was registered without an ID.");
      }

      if (mode === "mirror_active_local" && activeLocalLibrary) {
        await actions.switchLibrary(activeLocalLibrary.id);
        await api.setRemoteMirror(activeLocalLibrary.id, remoteLibraryId);
        await actions.initialize();
        setMessage(
          `Remote library created and now mirroring ${activeLocalLibrary.display_name}.`,
        );
      } else {
        await actions.switchLibrary(remoteLibraryId);
        setMessage("Remote library connected.");
      }

      onClose();
    } catch (err: unknown) {
      setError(err instanceof Error ? err.message : String(err));
    } finally {
      setLoading(false);
    }
  };

  const remoteLibraries = state.libraries.filter(
    (library): library is Extract<RegisteredLibrary, { kind: "remote" }> =>
      library.kind === "remote",
  );

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/60 p-4">
      <div className="w-full max-w-xl rounded-xl border border-[var(--color-border-light)] bg-[var(--color-sidebar)] p-5 shadow-2xl">
        <div className="mb-4 flex items-start justify-between gap-4">
          <div>
            <h2 className="text-lg font-semibold text-white">
              {t("settings.library.addRemoteLibrary", {
                defaultValue: "Add Remote Library",
              })}
            </h2>
            <p className="mt-1 text-sm text-[var(--color-text-dim)]">
              {t("settings.library.addRemoteLibraryDescription", {
                defaultValue:
                  "Open an existing remote library, or create one and mirror the active local library.",
              })}
            </p>
          </div>
          <button
            onClick={onClose}
            disabled={loading}
            className="rounded-md p-1 text-[var(--color-text-dim)] transition-colors hover:bg-[var(--color-hover)] hover:text-white"
          >
            <X size={16} />
          </button>
        </div>

        <div className="grid gap-3 md:grid-cols-2">
          <button
            onClick={() => setMode("open_remote")}
            disabled={loading}
            className={`rounded-lg border px-4 py-3 text-left ${
              mode === "open_remote"
                ? "border-[var(--color-accent)] bg-[var(--color-accent)]/10"
                : "border-[var(--color-border-light)] bg-[var(--color-surface)]"
            }`}
          >
            <p className="text-sm font-medium text-white">
              Open Remote Library
            </p>
            <p className="mt-1 text-xs text-[var(--color-text-dim)]">
              Register a remote working copy and switch into it.
            </p>
          </button>
          <button
            onClick={() => setMode("mirror_active_local")}
            disabled={loading || !canMirrorActiveLocal}
            className={`rounded-lg border px-4 py-3 text-left ${
              mode === "mirror_active_local"
                ? "border-[var(--color-accent)] bg-[var(--color-accent)]/10"
                : "border-[var(--color-border-light)] bg-[var(--color-surface)]"
            } disabled:opacity-50`}
          >
            <p className="text-sm font-medium text-white">
              Create And Mirror Active Local
            </p>
            <p className="mt-1 text-xs text-[var(--color-text-dim)]">
              {activeLocalLibrary
                ? `Mirror ${activeLocalLibrary.display_name} into a new remote library.`
                : "Switch to the local library you want to mirror first."}
            </p>
          </button>
        </div>

        <div className="mt-4 grid gap-3 md:grid-cols-3">
          {(
            [
              ["google_drive", Cloud],
              ["dropbox", Globe],
              ["webdav", FolderOpen],
            ] as const
          ).map(([candidate, Icon]) => (
            <button
              key={candidate}
              onClick={() => resetProviderState(candidate)}
              disabled={loading}
              className={`rounded-lg border px-4 py-3 text-left ${
                provider === candidate
                  ? "border-[var(--color-accent)] bg-[var(--color-accent)]/10"
                  : "border-[var(--color-border-light)] bg-[var(--color-surface)]"
              }`}
            >
              <Icon size={16} className="mb-2 text-[var(--color-accent)]" />
              <p className="text-sm font-medium text-white">
                {candidate === "google_drive"
                  ? "Google Drive"
                  : candidate === "dropbox"
                    ? "Dropbox"
                    : "WebDAV"}
              </p>
            </button>
          ))}
        </div>

        <div className="mt-4 space-y-3 rounded-lg border border-[var(--color-border-light)] bg-[var(--color-surface)] p-4">
          <div>
            <label className="mb-1 block text-xs font-medium text-white">
              Display name
            </label>
            <input
              value={displayName}
              onChange={(event) => setDisplayName(event.target.value)}
              className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-sidebar)] px-3 py-2 text-sm text-white outline-none focus:border-[var(--color-accent)]"
            />
          </div>

          {provider === "google_drive" && (
            <p className="text-xs text-[var(--color-text-dim)]">
              OpenKara uses its bundled Google Drive app registration and will
              create or reuse a folder with this display name in My Drive.
            </p>
          )}

          {provider === "dropbox" && (
            <p className="text-xs text-[var(--color-text-dim)]">
              OpenKara uses its bundled Dropbox app registration and will create
              or reuse a folder with this display name in Dropbox.
            </p>
          )}

          {provider === "webdav" && (
            <>
              <div>
                <label className="mb-1 block text-xs font-medium text-white">
                  Server URL
                </label>
                <input
                  value={serverUrl}
                  onChange={(event) => setServerUrl(event.target.value)}
                  placeholder="https://dav.example.com/remote.php/dav/files/you/"
                  className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-sidebar)] px-3 py-2 text-sm text-white outline-none focus:border-[var(--color-accent)]"
                  spellCheck={false}
                />
              </div>
              <div>
                <label className="mb-1 block text-xs font-medium text-white">
                  Library path
                </label>
                <input
                  value={rootPath}
                  onChange={(event) => setRootPath(event.target.value)}
                  placeholder="/OpenKara"
                  className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-sidebar)] px-3 py-2 text-sm text-white outline-none focus:border-[var(--color-accent)]"
                  spellCheck={false}
                />
              </div>
              <div className="grid gap-3 md:grid-cols-2">
                <div>
                  <label className="mb-1 block text-xs font-medium text-white">
                    Username
                  </label>
                  <input
                    value={username}
                    onChange={(event) => setUsername(event.target.value)}
                    className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-sidebar)] px-3 py-2 text-sm text-white outline-none focus:border-[var(--color-accent)]"
                    spellCheck={false}
                  />
                </div>
                <div>
                  <label className="mb-1 block text-xs font-medium text-white">
                    Password
                  </label>
                  <input
                    type="password"
                    value={password}
                    onChange={(event) => setPassword(event.target.value)}
                    className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-sidebar)] px-3 py-2 text-sm text-white outline-none focus:border-[var(--color-accent)]"
                  />
                </div>
              </div>
            </>
          )}

          <button
            onClick={() => void connect()}
            disabled={loading || meta.isInitializing}
            className="w-full rounded-lg bg-[var(--color-accent)] px-4 py-2.5 text-sm font-medium text-white transition-opacity hover:opacity-90 disabled:opacity-60"
          >
            {loading
              ? "Connecting…"
              : mode === "mirror_active_local"
                ? "Create Remote Library And Start Mirror"
                : "Open Remote Library"}
          </button>

          {authorizationUrl && (
            <a
              href={authorizationUrl}
              target="_blank"
              rel="noreferrer"
              className="block text-xs text-[var(--color-accent)] underline underline-offset-2"
            >
              Open browser sign-in again
            </a>
          )}

          {error && <p className="text-sm text-red-400">{error}</p>}
          {message && (
            <p className="text-sm text-[var(--color-text-dim)]">{message}</p>
          )}
        </div>

        {remoteLibraries.length > 0 && (
          <div className="mt-4 rounded-lg border border-[var(--color-border-light)] bg-[var(--color-surface)] p-4">
            <p className="mb-2 text-xs font-medium uppercase tracking-wide text-[var(--color-text-dim)]">
              Existing Remote Libraries
            </p>
            <div className="space-y-2">
              {remoteLibraries.map((library) => (
                <div
                  key={library.id}
                  className="rounded-md border border-[var(--color-border-light)] px-3 py-2"
                >
                  <p className="text-sm text-white">{library.display_name}</p>
                  <p className="text-xs text-[var(--color-text-dim)]">
                    {library.remote_path_display || library.remote_root_locator}
                  </p>
                </div>
              ))}
            </div>
          </div>
        )}
      </div>
    </div>
  );
}
