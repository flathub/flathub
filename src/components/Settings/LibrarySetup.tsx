import { useEffect, useState } from "react";
import { useTranslation } from "react-i18next";
import { open } from "@tauri-apps/plugin-dialog";
import {
  type LucideIcon,
  Cloud,
  FolderOpen,
  Plus,
  Music,
  Globe,
  Layers,
  Mic2,
  ChevronLeft,
  Check,
} from "lucide-react";
import * as api from "@/lib/tauri";
import i18next, { SUPPORTED_LANGUAGES, detectSystemLanguage } from "@/lib/i18n";
import { useSettingsStore } from "@/stores/settings-store";
import type { RemoteLibraryProvider } from "@/types/ipc";

type Step = "language" | "library" | "remoteProvider" | "stemMode";
type LibraryChoiceKind = "create_local" | "open_local" | "open_remote";

interface RemoteProviderChoice {
  provider: RemoteLibraryProvider;
  icon: LucideIcon;
  title: string;
  description: string;
  availableNow: boolean;
}

interface LibraryChoice {
  kind: LibraryChoiceKind;
  icon: LucideIcon;
  title: string;
  description: string;
}

const remoteProviderDisplayNames: Record<RemoteLibraryProvider, string> = {
  google_drive: "Google Drive Library",
  dropbox: "Dropbox Library",
  webdav: "WebDAV Library",
};

// eslint-disable-next-line react-refresh/only-export-components
export const librarySetupChoices: LibraryChoice[] = [
  {
    kind: "create_local",
    icon: Plus,
    title: "setup.createNew",
    description: "setup.createNewDescription",
  },
  {
    kind: "open_local",
    icon: FolderOpen,
    title: "setup.openExisting",
    description: "setup.openExistingDescription",
  },
  {
    kind: "open_remote",
    icon: Globe,
    title: "setup.openRemoteLibrary",
    description: "setup.openRemoteLibraryDescription",
  },
];

// eslint-disable-next-line react-refresh/only-export-components
export const remoteLibraryProviders: RemoteProviderChoice[] = [
  {
    provider: "google_drive",
    icon: Cloud,
    title: "Google Drive",
    description:
      "Connect through Google OAuth and keep an OpenKara library in My Drive.",
    availableNow: true,
  },
  {
    provider: "dropbox",
    icon: Globe,
    title: "Dropbox",
    description:
      "Planned next: browser sign-in and Dropbox-backed remote libraries.",
    availableNow: false,
  },
  {
    provider: "webdav",
    icon: FolderOpen,
    title: "WebDAV",
    description: "Connect to a WebDAV-hosted OpenKara library right now.",
    availableNow: true,
  },
];

interface LibrarySetupProps {
  onComplete: () => void;
}

function StepIndicator({ current }: { current: Step }) {
  const steps: Step[] = ["language", "library", "remoteProvider", "stemMode"];
  const currentIndex = steps.indexOf(current);

  return (
    <div className="flex items-center justify-center gap-2">
      {steps.map((step, i) => (
        <div
          key={step}
          className={`h-1.5 w-1.5 rounded-full transition-colors ${
            i <= currentIndex
              ? "bg-[var(--color-accent)]"
              : "bg-[var(--color-border)]"
          }`}
        />
      ))}
    </div>
  );
}

export function LibrarySetup({ onComplete }: LibrarySetupProps) {
  const { t } = useTranslation();
  const settingsHydrated = useSettingsStore((s) => s.hydrated);
  const settingsLanguage = useSettingsStore((s) => s.language);
  const settingsStemMode = useSettingsStore((s) => s.stemMode);
  const patchAppSettings = useSettingsStore((s) => s.patchAppSettings);
  const hydrateAppSettings = useSettingsStore((s) => s.hydrateAppSettings);
  const [step, setStep] = useState<Step>("language");
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);
  const [selectedRemoteProvider, setSelectedRemoteProvider] =
    useState<RemoteLibraryProvider | null>(null);
  const [remoteMessage, setRemoteMessage] = useState<string | null>(null);
  const [remoteAuthorizationUrl, setRemoteAuthorizationUrl] = useState<
    string | null
  >(null);
  const [remoteDisplayName, setRemoteDisplayName] = useState("WebDAV Library");
  const [googleClientId, setGoogleClientId] = useState("");
  const [googleClientSecret, setGoogleClientSecret] = useState("");
  const [remoteServerUrl, setRemoteServerUrl] = useState("");
  const [remoteUsername, setRemoteUsername] = useState("");
  const [remotePassword, setRemotePassword] = useState("");
  const [remoteRootPath, setRemoteRootPath] = useState("/OpenKara");
  const [selectedLanguage, setSelectedLanguage] = useState(
    () =>
      settingsLanguage ?? i18next.resolvedLanguage ?? detectSystemLanguage(),
  );
  const [selectedStemMode, setSelectedStemMode] = useState<
    "two_stem" | "four_stem"
  >(settingsStemMode);

  const resolveSingleDirectory = (selected: string | string[] | null) =>
    typeof selected === "string" ? selected : (selected?.[0] ?? null);

  const resetRemoteWizard = () => {
    setSelectedRemoteProvider(null);
    setRemoteMessage(null);
    setRemoteAuthorizationUrl(null);
    setRemoteDisplayName("WebDAV Library");
    setGoogleClientId("");
    setGoogleClientSecret("");
    setRemoteServerUrl("");
    setRemoteUsername("");
    setRemotePassword("");
    setRemoteRootPath("/OpenKara");
  };

  useEffect(() => {
    if (!settingsHydrated) {
      return;
    }

    setSelectedLanguage(
      settingsLanguage ?? i18next.resolvedLanguage ?? detectSystemLanguage(),
    );
  }, [settingsHydrated, settingsLanguage]);

  useEffect(() => {
    if (!settingsHydrated) {
      return;
    }

    setSelectedStemMode(settingsStemMode);
  }, [settingsHydrated, settingsStemMode]);

  const handleLanguageSelect = (code: string) => {
    setSelectedLanguage(code);
    patchAppSettings({ language: code });
    i18next.changeLanguage(code);
    api
      .setLanguage(code)
      .then(hydrateAppSettings)
      .catch(() => {
        // non-fatal: language saved on next step anyway
      });
    setStep("library");
  };

  const handleCreate = async () => {
    const selected = await open({
      directory: true,
      title: t("setup.dialogTitleCreate"),
    });

    if (!selected) return;
    const selectedDirectory = resolveSingleDirectory(selected);
    if (!selectedDirectory) return;

    const libraryDir = `${selectedDirectory}/OpenKara`;
    setLoading(true);
    setError(null);
    try {
      await api.createLocalLibrary(libraryDir);
      setStep("stemMode");
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : String(err);
      setError(message);
    } finally {
      setLoading(false);
    }
  };

  const handleOpen = async () => {
    const selected = await open({
      directory: true,
      title: t("setup.dialogTitleOpen"),
    });

    if (!selected) return;
    const selectedDirectory = resolveSingleDirectory(selected);
    if (!selectedDirectory) return;

    setLoading(true);
    setError(null);
    try {
      await api.registerLocalLibrary(selectedDirectory);
      setStep("stemMode");
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : String(err);
      setError(message);
    } finally {
      setLoading(false);
    }
  };

  const handleOpenRemote = () => {
    resetRemoteWizard();
    setError(null);
    setStep("remoteProvider");
  };

  const connectRemoteLibrary = async (provider: RemoteLibraryProvider) => {
    setError(null);
    setRemoteMessage(null);
    setRemoteAuthorizationUrl(null);
    setSelectedRemoteProvider(provider);
    setLoading(true);
    try {
      const start = await api.beginRemoteAuth(
        provider,
        provider === "google_drive"
          ? {
              type: "google_drive",
              client_id: googleClientId.trim(),
              client_secret: googleClientSecret.trim() || null,
            }
          : provider === "webdav"
            ? {
                type: "webdav",
                server_url: remoteServerUrl,
                username: remoteUsername,
                password: remotePassword,
                root_path: remoteRootPath.trim() || null,
              }
            : null,
      );

      if (start.authorization_url) {
        setRemoteAuthorizationUrl(start.authorization_url);
        setRemoteMessage(
          provider === "google_drive"
            ? "Google sign-in opened in your browser. Finish the consent flow and OpenKara will continue automatically."
            : null,
        );
        globalThis.open?.(
          start.authorization_url,
          "_blank",
          "noopener,noreferrer",
        );

        const deadline = Date.now() + 120_000;
        let ready = false;
        while (Date.now() < deadline) {
          await new Promise((resolve) => window.setTimeout(resolve, 1000));
          const status = await api.pollRemoteAuth(start.session_id);
          if (status.state === "ready") {
            ready = true;
            break;
          }
          if (status.state === "failed") {
            throw new Error(
              status.error?.message ?? "Remote sign-in failed unexpectedly.",
            );
          }
        }

        if (!ready) {
          throw new Error(
            provider === "google_drive"
              ? "Google sign-in timed out before OpenKara received the callback."
              : "Remote sign-in timed out.",
          );
        }
      }

      const candidate = await api.createRemoteLibrary(
        start.session_id,
        remoteDisplayName.trim() || remoteProviderDisplayNames[provider],
      );
      await api.registerRemoteLibrary(
        start.session_id,
        candidate.remote_root_locator,
        remoteDisplayName.trim() || candidate.display_name,
      );
      setRemoteMessage(
        t("setup.remoteAuthStarted", {
          defaultValue:
            provider === "google_drive"
              ? "Google Drive library connected successfully."
              : provider === "webdav"
                ? "WebDAV library connected successfully."
                : "Remote library connected successfully.",
        }),
      );
      setStep("stemMode");
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : String(err);
      setError(message);
    } finally {
      setLoading(false);
    }
  };

  const handleWebDavConnect = async () => {
    if (!remoteServerUrl.trim()) {
      setError("Enter the WebDAV server URL first.");
      return;
    }

    if (!remoteUsername.trim()) {
      setError("Enter the WebDAV username first.");
      return;
    }

    if (!remotePassword.trim()) {
      setError("Enter the WebDAV password first.");
      return;
    }

    await connectRemoteLibrary("webdav");
  };

  const handleGoogleDriveConnect = async () => {
    if (!googleClientId.trim()) {
      setError("Enter the Google OAuth client ID first.");
      return;
    }

    await connectRemoteLibrary("google_drive");
  };

  const handleFinish = async () => {
    try {
      const settings = await api.setStemMode(selectedStemMode);
      hydrateAppSettings(settings);
    } catch {
      // non-fatal
    }
    onComplete();
  };

  return (
    <div className="flex h-screen w-full items-center justify-center bg-[var(--color-surface)]">
      <div className="mx-auto max-w-md space-y-8 px-6 text-center">
        {/* Step indicator */}
        <StepIndicator current={step} />

        {/* Step 1: Language */}
        {step === "language" && (
          <>
            <div className="flex flex-col items-center gap-4">
              <div className="flex h-16 w-16 items-center justify-center rounded-2xl bg-[var(--color-accent)]/15">
                <Globe size={32} className="text-[var(--color-accent)]" />
              </div>
              <h1 className="text-2xl font-bold text-white">
                {t("setup.chooseLanguage")}
              </h1>
            </div>

            <div className="space-y-3">
              {SUPPORTED_LANGUAGES.map((lang) => (
                <button
                  key={lang.code}
                  onClick={() => handleLanguageSelect(lang.code)}
                  className={`flex w-full items-center gap-3 rounded-lg border px-5 py-4 text-left transition-colors ${
                    selectedLanguage === lang.code
                      ? "border-[var(--color-accent)] bg-[var(--color-accent)]/10"
                      : "border-[var(--color-border-light)] bg-[var(--color-sidebar)] hover:bg-[var(--color-hover)]"
                  }`}
                >
                  <div className="flex h-8 w-8 items-center justify-center rounded-full bg-[var(--color-hover)]">
                    <span className="text-[14px] font-medium text-white">
                      {lang.code === "en" ? "EN" : "中"}
                    </span>
                  </div>
                  <span className="text-[14px] font-medium text-white">
                    {lang.name}
                  </span>
                  {selectedLanguage === lang.code && (
                    <Check
                      size={16}
                      className="ml-auto text-[var(--color-accent)]"
                    />
                  )}
                </button>
              ))}
            </div>
          </>
        )}

        {/* Step 2: Library Location */}
        {step === "library" && (
          <>
            <div className="flex flex-col items-center gap-4">
              <div className="flex h-16 w-16 items-center justify-center rounded-2xl bg-[var(--color-accent)]/15">
                <Music size={32} className="text-[var(--color-accent)]" />
              </div>
              <h1 className="text-2xl font-bold text-white">
                {t("setup.welcome")}
              </h1>
              <p className="text-[14px] leading-relaxed text-[var(--color-text-dim)]">
                {t("setup.description")}
              </p>
            </div>

            <div className="space-y-3">
              {librarySetupChoices.map((choice) => {
                const Icon = choice.icon;
                const disabled = loading;

                return (
                  <button
                    key={choice.kind}
                    onClick={
                      choice.kind === "create_local"
                        ? handleCreate
                        : choice.kind === "open_local"
                          ? handleOpen
                          : handleOpenRemote
                    }
                    disabled={disabled}
                    className="flex w-full items-center gap-3 rounded-lg border border-[var(--color-border-light)] bg-[var(--color-sidebar)] px-5 py-4 text-left transition-colors hover:bg-[var(--color-hover)] disabled:opacity-50"
                  >
                    <Icon
                      size={20}
                      className={`shrink-0 ${
                        choice.kind === "open_remote"
                          ? "text-[var(--color-accent)]"
                          : choice.kind === "create_local"
                            ? "text-[var(--color-accent)]"
                            : "text-[var(--color-text-dim)]"
                      }`}
                    />
                    <div>
                      <div className="text-[14px] font-medium text-white">
                        {t(choice.title, {
                          defaultValue:
                            choice.kind === "create_local"
                              ? "Create new local library"
                              : choice.kind === "open_local"
                                ? "Open existing local library"
                                : "Open remote library",
                        })}
                      </div>
                      <div className="text-[12px] text-[var(--color-text-dim)]">
                        {t(choice.description, {
                          defaultValue:
                            choice.kind === "create_local"
                              ? "Create a new local library folder on this machine."
                              : choice.kind === "open_local"
                                ? "Register an existing local library folder."
                                : "Connect to a cloud-hosted library without copying the original files.",
                        })}
                      </div>
                    </div>
                  </button>
                );
              })}
            </div>

            {error && <p className="text-[13px] text-red-400">{error}</p>}

            {loading && (
              <p className="text-[13px] text-[var(--color-text-dim)]">
                {t("setup.settingUp")}
              </p>
            )}

            <button
              onClick={() => setStep("language")}
              className="flex items-center justify-center gap-1 text-[13px] text-[var(--color-text-dim)] transition-colors hover:text-white"
            >
              <ChevronLeft size={14} />
              {t("setup.back")}
            </button>
          </>
        )}

        {/* Step 3: Remote Provider */}
        {step === "remoteProvider" && (
          <>
            <div className="flex flex-col items-center gap-4">
              <div className="flex h-16 w-16 items-center justify-center rounded-2xl bg-[var(--color-accent)]/15">
                <Cloud size={32} className="text-[var(--color-accent)]" />
              </div>
              <h1 className="text-2xl font-bold text-white">
                {t("setup.openRemoteLibrary", {
                  defaultValue: "Choose a remote provider",
                })}
              </h1>
              <p className="text-[14px] leading-relaxed text-[var(--color-text-dim)]">
                {t("setup.openRemoteLibraryDescription", {
                  defaultValue:
                    "Connect Google Drive or WebDAV to your remote library.",
                })}
              </p>
            </div>

            <div className="space-y-3">
              {remoteLibraryProviders.map((choice) => {
                const Icon = choice.icon;
                const isActive = selectedRemoteProvider === choice.provider;

                return (
                  <button
                    key={choice.provider}
                    onClick={() => {
                      setSelectedRemoteProvider(choice.provider);
                      setRemoteMessage(null);
                      setError(null);
                      setRemoteAuthorizationUrl(null);
                      setRemoteDisplayName(
                        remoteProviderDisplayNames[choice.provider],
                      );
                    }}
                    disabled={loading}
                    className={`flex w-full items-start gap-3 rounded-lg border px-5 py-4 text-left transition-colors disabled:opacity-50 ${
                      isActive
                        ? "border-[var(--color-accent)] bg-[var(--color-accent)]/10"
                        : "border-[var(--color-border-light)] bg-[var(--color-sidebar)] hover:bg-[var(--color-hover)]"
                    }`}
                  >
                    <Icon
                      size={20}
                      className="mt-0.5 shrink-0 text-[var(--color-accent)]"
                    />
                    <div className="min-w-0 flex-1">
                      <div className="text-[14px] font-medium text-white">
                        {choice.title}
                      </div>
                      <div className="text-[12px] text-[var(--color-text-dim)]">
                        {choice.description}
                      </div>
                      <div className="mt-1 text-[11px] text-[var(--color-text-dimmer)]">
                        {choice.availableNow
                          ? "Available in this build"
                          : "Planned in the next provider phase"}
                      </div>
                    </div>
                    {isActive && (
                      <Check
                        size={16}
                        className="mt-0.5 shrink-0 text-[var(--color-accent)]"
                      />
                    )}
                  </button>
                );
              })}
            </div>

            {selectedRemoteProvider === "google_drive" && (
              <div className="space-y-3 rounded-lg border border-[var(--color-border-light)] bg-[var(--color-sidebar)] p-4 text-left">
                <div>
                  <label className="mb-1 block text-[12px] font-medium text-white">
                    Display name
                  </label>
                  <input
                    value={remoteDisplayName}
                    onChange={(event) =>
                      setRemoteDisplayName(event.target.value)
                    }
                    placeholder="Google Drive Library"
                    className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-2 text-[13px] text-white outline-none transition-colors focus:border-[var(--color-accent)]"
                  />
                </div>

                <div>
                  <label className="mb-1 block text-[12px] font-medium text-white">
                    OAuth client ID
                  </label>
                  <input
                    value={googleClientId}
                    onChange={(event) => setGoogleClientId(event.target.value)}
                    placeholder="1234567890-abc.apps.googleusercontent.com"
                    className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-2 text-[13px] text-white outline-none transition-colors focus:border-[var(--color-accent)]"
                    spellCheck={false}
                  />
                </div>

                <div>
                  <label className="mb-1 block text-[12px] font-medium text-white">
                    OAuth client secret (optional)
                  </label>
                  <input
                    value={googleClientSecret}
                    onChange={(event) =>
                      setGoogleClientSecret(event.target.value)
                    }
                    placeholder="optional for some desktop clients"
                    className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-2 text-[13px] text-white outline-none transition-colors focus:border-[var(--color-accent)]"
                    spellCheck={false}
                  />
                  <p className="mt-1 text-[11px] text-[var(--color-text-dimmer)]">
                    OpenKara will create or reuse a folder with the display name
                    above in My Drive, then keep the remote library there.
                  </p>
                </div>

                <button
                  onClick={() => void handleGoogleDriveConnect()}
                  disabled={loading}
                  className="w-full rounded-lg bg-[var(--color-accent)] px-4 py-2.5 text-[13px] font-medium text-white transition-opacity hover:opacity-90 disabled:opacity-60"
                >
                  {loading ? "Waiting for Google…" : "Connect Google Drive"}
                </button>

                {remoteAuthorizationUrl && (
                  <a
                    href={remoteAuthorizationUrl}
                    target="_blank"
                    rel="noreferrer"
                    className="block text-[12px] text-[var(--color-accent)] underline underline-offset-2"
                  >
                    Open Google sign-in in your browser again
                  </a>
                )}
              </div>
            )}

            {selectedRemoteProvider === "webdav" && (
              <div className="space-y-3 rounded-lg border border-[var(--color-border-light)] bg-[var(--color-sidebar)] p-4 text-left">
                <div>
                  <label className="mb-1 block text-[12px] font-medium text-white">
                    Display name
                  </label>
                  <input
                    value={remoteDisplayName}
                    onChange={(event) =>
                      setRemoteDisplayName(event.target.value)
                    }
                    placeholder="WebDAV Library"
                    className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-2 text-[13px] text-white outline-none transition-colors focus:border-[var(--color-accent)]"
                  />
                </div>

                <div>
                  <label className="mb-1 block text-[12px] font-medium text-white">
                    Server URL
                  </label>
                  <input
                    value={remoteServerUrl}
                    onChange={(event) => setRemoteServerUrl(event.target.value)}
                    placeholder="https://dav.example.com/remote.php/dav/files/you/"
                    className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-2 text-[13px] text-white outline-none transition-colors focus:border-[var(--color-accent)]"
                    spellCheck={false}
                  />
                </div>

                <div>
                  <label className="mb-1 block text-[12px] font-medium text-white">
                    Library path
                  </label>
                  <input
                    value={remoteRootPath}
                    onChange={(event) => setRemoteRootPath(event.target.value)}
                    placeholder="/OpenKara"
                    className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-2 text-[13px] text-white outline-none transition-colors focus:border-[var(--color-accent)]"
                    spellCheck={false}
                  />
                  <p className="mt-1 text-[11px] text-[var(--color-text-dimmer)]">
                    Point this at an existing remote library path, or enter a
                    new folder name and OpenKara will initialize it for you.
                  </p>
                </div>

                <div className="grid grid-cols-1 gap-3 sm:grid-cols-2">
                  <div>
                    <label className="mb-1 block text-[12px] font-medium text-white">
                      Username
                    </label>
                    <input
                      value={remoteUsername}
                      onChange={(event) =>
                        setRemoteUsername(event.target.value)
                      }
                      placeholder="username"
                      className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-2 text-[13px] text-white outline-none transition-colors focus:border-[var(--color-accent)]"
                      spellCheck={false}
                    />
                  </div>
                  <div>
                    <label className="mb-1 block text-[12px] font-medium text-white">
                      Password
                    </label>
                    <input
                      type="password"
                      value={remotePassword}
                      onChange={(event) =>
                        setRemotePassword(event.target.value)
                      }
                      placeholder="password"
                      className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-2 text-[13px] text-white outline-none transition-colors focus:border-[var(--color-accent)]"
                    />
                  </div>
                </div>

                <button
                  onClick={() => void handleWebDavConnect()}
                  disabled={loading}
                  className="w-full rounded-lg bg-[var(--color-accent)] px-4 py-2.5 text-[13px] font-medium text-white transition-opacity hover:opacity-90 disabled:opacity-60"
                >
                  {loading ? "Connecting…" : "Connect WebDAV library"}
                </button>
              </div>
            )}

            {selectedRemoteProvider === "dropbox" && (
              <div className="rounded-lg border border-[var(--color-border-light)] bg-[var(--color-sidebar)] px-4 py-3 text-left text-[12px] text-[var(--color-text-dim)]">
                Dropbox still stays in the completion plan, but it is not wired
                yet in this build. OpenKara now keeps Google Drive and WebDAV as
                the real remote-library paths instead of pretending Dropbox is
                ready before the provider code exists.
              </div>
            )}

            {selectedRemoteProvider && remoteMessage && (
              <p className="text-[13px] text-[var(--color-text-dim)]">
                {remoteMessage}
              </p>
            )}

            {error && <p className="text-[13px] text-red-400">{error}</p>}

            <button
              onClick={() => {
                resetRemoteWizard();
                setStep("library");
              }}
              className="flex items-center justify-center gap-1 text-[13px] text-[var(--color-text-dim)] transition-colors hover:text-white"
            >
              <ChevronLeft size={14} />
              {t("setup.back")}
            </button>
          </>
        )}

        {/* Step 3: Stem Mode */}
        {step === "stemMode" && (
          <>
            <div className="flex flex-col items-center gap-4">
              <div className="flex h-16 w-16 items-center justify-center rounded-2xl bg-[var(--color-accent)]/15">
                <Layers size={32} className="text-[var(--color-accent)]" />
              </div>
              <h1 className="text-2xl font-bold text-white">
                {t("setup.chooseStemMode")}
              </h1>
              <p className="text-[14px] leading-relaxed text-[var(--color-text-dim)]">
                {t("setup.stemModeDescription")}
              </p>
            </div>

            <div className="space-y-3">
              <button
                onClick={() => setSelectedStemMode("two_stem")}
                className={`flex w-full items-center gap-3 rounded-lg border px-5 py-4 text-left transition-colors ${
                  selectedStemMode === "two_stem"
                    ? "border-[var(--color-accent)] bg-[var(--color-accent)]/10"
                    : "border-[var(--color-border-light)] bg-[var(--color-sidebar)] hover:bg-[var(--color-hover)]"
                }`}
              >
                <Mic2
                  size={20}
                  className="shrink-0 text-[var(--color-accent)]"
                />
                <div className="flex-1">
                  <div className="text-[14px] font-medium text-white">
                    {t("setup.twoStem")}
                  </div>
                  <div className="text-[12px] text-[var(--color-text-dim)]">
                    {t("setup.twoStemSubtitle")}
                  </div>
                  <div className="mt-0.5 text-[11px] text-[var(--color-text-dimmer)]">
                    {t("setup.twoStemDetail")}
                  </div>
                </div>
                {selectedStemMode === "two_stem" && (
                  <Check
                    size={16}
                    className="shrink-0 text-[var(--color-accent)]"
                  />
                )}
              </button>

              <button
                onClick={() => setSelectedStemMode("four_stem")}
                className={`flex w-full items-center gap-3 rounded-lg border px-5 py-4 text-left transition-colors ${
                  selectedStemMode === "four_stem"
                    ? "border-[var(--color-accent)] bg-[var(--color-accent)]/10"
                    : "border-[var(--color-border-light)] bg-[var(--color-sidebar)] hover:bg-[var(--color-hover)]"
                }`}
              >
                <Layers
                  size={20}
                  className="shrink-0 text-[var(--color-accent)]"
                />
                <div className="flex-1">
                  <div className="text-[14px] font-medium text-white">
                    {t("setup.fourStem")}
                  </div>
                  <div className="text-[12px] text-[var(--color-text-dim)]">
                    {t("setup.fourStemSubtitle")}
                  </div>
                  <div className="mt-0.5 text-[11px] text-[var(--color-text-dimmer)]">
                    {t("setup.fourStemDetail")}
                  </div>
                </div>
                {selectedStemMode === "four_stem" && (
                  <Check
                    size={16}
                    className="shrink-0 text-[var(--color-accent)]"
                  />
                )}
              </button>
            </div>

            <button
              onClick={handleFinish}
              className="w-full rounded-lg bg-[var(--color-accent)] px-5 py-3 text-[14px] font-medium text-white transition-opacity hover:opacity-90"
            >
              {t("setup.getStarted")}
            </button>

            <button
              onClick={() => setStep("library")}
              className="flex items-center justify-center gap-1 text-[13px] text-[var(--color-text-dim)] transition-colors hover:text-white"
            >
              <ChevronLeft size={14} />
              {t("setup.back")}
            </button>
          </>
        )}
      </div>
    </div>
  );
}
