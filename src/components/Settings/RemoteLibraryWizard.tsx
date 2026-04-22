import { useEffect, useRef, useState } from "react";
import { useTranslation } from "react-i18next";
import { Cloud, X } from "lucide-react";
import { getErrorMessage } from "@/lib/errors";
import * as api from "@/lib/tauri";
import type { RegisteredLibrary, RemoteLibraryProvider } from "@/types/ipc";
import { useSettingsOverlay } from "./SettingsOverlay.context";

type RemoteSetupMode = "open_remote" | "mirror_active_local";

function getRemoteProviderDisplayName(
  t: ReturnType<typeof useTranslation>["t"],
  provider: RemoteLibraryProvider,
) {
  return provider === "google_drive"
    ? t("settings.library.googleDriveLibraryName", {
        defaultValue: "Google Drive Library",
      })
    : provider === "dropbox"
      ? t("settings.library.dropboxLibraryName", {
          defaultValue: "Dropbox Library",
        })
      : t("settings.library.webdavLibraryName", {
          defaultValue: "WebDAV Library",
        });
}

function getRemoteProviderLabel(
  t: ReturnType<typeof useTranslation>["t"],
  provider: RemoteLibraryProvider,
) {
  return provider === "google_drive"
    ? t("setup.remoteProvider.googleDrive.title", {
        defaultValue: "Google Drive",
      })
    : provider === "dropbox"
      ? t("setup.remoteProvider.dropbox.title", {
          defaultValue: "Dropbox",
        })
      : t("setup.remoteProvider.webdav.title", {
          defaultValue: "WebDAV",
        });
}

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
  const [authSessionId, setAuthSessionId] = useState<string | null>(null);
  const [displayName, setDisplayName] = useState(
    getRemoteProviderDisplayName(t, "google_drive"),
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
  const cancelledRef = useRef(false);
  const mountedRef = useRef(true);

  useEffect(() => {
    return () => {
      cancelledRef.current = true;
      mountedRef.current = false;
    };
  }, []);

  const resetProviderState = (nextProvider: RemoteLibraryProvider) => {
    setProvider(nextProvider);
    setDisplayName(getRemoteProviderDisplayName(t, nextProvider));
    setError(null);
    setMessage(null);
    setAuthorizationUrl(null);
    setAuthSessionId(null);
  };

  const requestClose = () => {
    cancelledRef.current = true;
    if (authSessionId) {
      void api.cancelRemoteAuth(authSessionId);
    }
    if (mountedRef.current) {
      setLoading(false);
    }
    onClose();
  };

  const completeBrowserAuth = async (
    sessionId: string,
    nextProvider: RemoteLibraryProvider,
  ) => {
    const deadline = Date.now() + 120_000;
    while (Date.now() < deadline) {
      if (cancelledRef.current) {
        throw new Error("__remote_auth_cancelled__");
      }
      await new Promise((resolve) => window.setTimeout(resolve, 1000));
      if (cancelledRef.current) {
        throw new Error("__remote_auth_cancelled__");
      }
      const status = await api.pollRemoteAuth(sessionId);
      if (status.state === "ready") {
        return;
      }
      if (status.state === "failed") {
        throw new Error(
          status.error?.message ??
            t("settings.library.remoteSignInFailedUnexpectedly", {
              defaultValue: "Remote sign-in failed unexpectedly.",
            }),
        );
      }
    }

    throw new Error(
      nextProvider === "google_drive"
        ? t("settings.library.googleSignInTimedOut", {
            defaultValue:
              "Google sign-in timed out before OpenKara received the callback.",
          })
        : nextProvider === "dropbox"
          ? t("settings.library.dropboxSignInTimedOut", {
              defaultValue:
                "Dropbox sign-in timed out before OpenKara received the callback.",
            })
          : t("settings.library.remoteSignInTimedOut", {
              defaultValue: "Remote sign-in timed out.",
            }),
    );
  };

  const connect = async () => {
    cancelledRef.current = false;
    let startedSessionId: string | null = null;
    if (provider === "webdav") {
      if (!serverUrl.trim()) {
        setError(
          t("settings.library.webdavEnterServerUrl", {
            defaultValue: "Enter the WebDAV server URL first.",
          }),
        );
        return;
      }
      if (!username.trim()) {
        setError(
          t("settings.library.webdavEnterUsername", {
            defaultValue: "Enter the WebDAV username first.",
          }),
        );
        return;
      }
      if (!password.trim()) {
        setError(
          t("settings.library.webdavEnterPassword", {
            defaultValue: "Enter the WebDAV password first.",
          }),
        );
        return;
      }
    }

    if (mode === "mirror_active_local" && !activeLocalLibrary) {
      setError(
        t("settings.library.mirrorActiveLocalDescriptionNoLocal", {
          defaultValue: "Switch to the local library you want to mirror first.",
        }),
      );
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
      startedSessionId = start.session_id;

      if (start.authorization_url) {
        setAuthSessionId(start.session_id);
        setAuthorizationUrl(start.authorization_url);
        await api.openExternalUrl(start.authorization_url);
        await completeBrowserAuth(start.session_id, provider);
      }

      if (cancelledRef.current) {
        return;
      }

      const candidate = await api.createRemoteLibrary(
        start.session_id,
        displayName.trim() || getRemoteProviderDisplayName(t, provider),
      );
      const registry = await api.registerRemoteLibrary(
        start.session_id,
        candidate.remote_root_locator,
        displayName.trim() || candidate.display_name,
      );
      const remoteLibraryId = registry.active_library_id;

      if (!remoteLibraryId) {
        throw new Error(
          t("settings.library.remoteLibraryMissingId", {
            defaultValue:
              "The new remote library was registered without an ID.",
          }),
        );
      }

      if (mode === "mirror_active_local" && activeLocalLibrary) {
        await actions.switchLibrary(activeLocalLibrary.id);
        await api.setRemoteMirror(activeLocalLibrary.id, remoteLibraryId);
        await actions.initialize();
        setMessage(
          t("settings.library.remoteLibraryCreatedAndMirroring", {
            defaultValue:
              "Remote library created and now mirroring {{displayName}}.",
            displayName: activeLocalLibrary.display_name,
          }),
        );
      } else {
        await actions.switchLibrary(remoteLibraryId);
        setMessage(
          t("settings.library.remoteLibraryConnected", {
            defaultValue: "Remote library connected.",
          }),
        );
      }

      onClose();
    } catch (err: unknown) {
      if (startedSessionId) {
        void api.cancelRemoteAuth(startedSessionId);
      }
      if (getErrorMessage(err) !== "__remote_auth_cancelled__") {
        setError(getErrorMessage(err));
      }
    } finally {
      if (mountedRef.current) {
        setAuthSessionId(null);
      }
      if (mountedRef.current && !cancelledRef.current) {
        setLoading(false);
      }
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
            onClick={requestClose}
            aria-label={t("common.close")}
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
              {t("settings.library.openRemoteLibrary", {
                defaultValue: "Open Remote Library",
              })}
            </p>
            <p className="mt-1 text-xs text-[var(--color-text-dim)]">
              {t("settings.library.openRemoteLibraryDescription", {
                defaultValue:
                  "Register a remote working copy and switch into it.",
              })}
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
              {t("settings.library.createAndMirrorActiveLocal", {
                defaultValue: "Create And Mirror Active Local",
              })}
            </p>
            <p className="mt-1 text-xs text-[var(--color-text-dim)]">
              {activeLocalLibrary
                ? t("settings.library.mirrorActiveLocalDescriptionWithName", {
                    defaultValue:
                      "Mirror {{displayName}} into a new remote library.",
                    displayName: activeLocalLibrary.display_name,
                  })
                : t("settings.library.mirrorActiveLocalDescriptionNoLocal", {
                    defaultValue:
                      "Switch to the local library you want to mirror first.",
                  })}
            </p>
          </button>
        </div>

        <div className="mt-4 grid gap-3 md:grid-cols-3">
          {(
            [
              ["google_drive", Cloud],
              ["dropbox", Cloud],
              ["webdav", Cloud],
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
                {getRemoteProviderLabel(t, candidate)}
              </p>
            </button>
          ))}
        </div>

        <div className="mt-4 space-y-3 rounded-lg border border-[var(--color-border-light)] bg-[var(--color-surface)] p-4">
          <div>
            <label className="mb-1 block text-xs font-medium text-white">
              {t("settings.library.displayName", {
                defaultValue: "Display name",
              })}
            </label>
            <input
              value={displayName}
              onChange={(event) => setDisplayName(event.target.value)}
              className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-sidebar)] px-3 py-2 text-sm text-white outline-none focus:border-[var(--color-accent)]"
            />
          </div>

          {provider === "google_drive" && (
            <p className="text-xs text-[var(--color-text-dim)]">
              {t("settings.library.googleDriveBundledDescription", {
                defaultValue:
                  "OpenKara uses its bundled Google Drive app registration and will create or reuse a folder with this display name in My Drive.",
              })}
            </p>
          )}

          {provider === "dropbox" && (
            <p className="text-xs text-[var(--color-text-dim)]">
              {t("settings.library.dropboxBundledDescription", {
                defaultValue:
                  "OpenKara uses its bundled Dropbox app registration and will create or reuse a folder with this display name in Dropbox.",
              })}
            </p>
          )}

          {provider === "webdav" && (
            <>
              <div>
                <label className="mb-1 block text-xs font-medium text-white">
                  {t("settings.library.webdavServerUrl", {
                    defaultValue: "Server URL",
                  })}
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
                  {t("settings.library.webdavLibraryPath", {
                    defaultValue: "Library path",
                  })}
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
                    {t("settings.library.webdavUsername", {
                      defaultValue: "Username",
                    })}
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
                    {t("settings.library.webdavPassword", {
                      defaultValue: "Password",
                    })}
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
              ? t("settings.library.connecting", {
                  defaultValue: "Connecting…",
                })
              : mode === "mirror_active_local"
                ? t("settings.library.createRemoteLibraryAndStartMirror", {
                    defaultValue: "Create Remote Library And Start Mirror",
                  })
                : t("settings.library.openRemoteLibrary", {
                    defaultValue: "Open Remote Library",
                  })}
          </button>

          {authorizationUrl && (
            <a
              href={authorizationUrl}
              target="_blank"
              rel="noreferrer"
              className="block text-xs text-[var(--color-accent)] underline underline-offset-2"
            >
              {t("settings.library.openBrowserSignInAgain", {
                defaultValue: "Open browser sign-in again",
              })}
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
              {t("settings.library.existingRemoteLibraries", {
                defaultValue: "Existing Remote Libraries",
              })}
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
