import { useEffect, useRef, useState } from "react";
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
import { getErrorMessage } from "@/lib/errors";
import * as api from "@/lib/tauri";
import i18next, { SUPPORTED_LANGUAGES, detectSystemLanguage } from "@/lib/i18n";
import { useSettingsStore } from "@/stores/settings-store";
import type { RemoteLibraryProvider } from "@/types/ipc";
import { runRemoteLibraryRegistrationFlow } from "./remote-library-flow";
import {
  getRemoteLibraryConnectedMessage,
  getRemoteProviderDisplayName,
} from "./remote-library-copy";

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
    title: "setup.remoteProvider.googleDrive.title",
    description: "setup.remoteProvider.googleDrive.description",
    availableNow: true,
  },
  {
    provider: "dropbox",
    icon: Cloud,
    title: "setup.remoteProvider.dropbox.title",
    description: "setup.remoteProvider.dropbox.description",
    availableNow: true,
  },
  {
    provider: "webdav",
    icon: Cloud,
    title: "setup.remoteProvider.webdav.title",
    description: "setup.remoteProvider.webdav.description",
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
  const [remoteDisplayName, setRemoteDisplayName] = useState(() =>
    getRemoteProviderDisplayName(t, "google_drive"),
  );
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
  const remoteAuthSessionIdRef = useRef<string | null>(null);

  const resolveSingleDirectory = (selected: string | string[] | null) =>
    typeof selected === "string" ? selected : (selected?.[0] ?? null);

  const resetRemoteWizard = () => {
    setSelectedRemoteProvider(null);
    setRemoteMessage(null);
    setRemoteAuthorizationUrl(null);
    remoteAuthSessionIdRef.current = null;
    setRemoteDisplayName(getRemoteProviderDisplayName(t, "google_drive"));
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

  useEffect(() => {
    return () => {
      if (remoteAuthSessionIdRef.current) {
        void api.cancelRemoteAuth(remoteAuthSessionIdRef.current);
      }
    };
  }, []);

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
      setError(getErrorMessage(err));
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
      setError(getErrorMessage(err));
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
      await runRemoteLibraryRegistrationFlow({
        provider,
        displayName: remoteDisplayName,
        t,
        webdav: {
          serverUrl: remoteServerUrl,
          username: remoteUsername,
          password: remotePassword,
          rootPath: remoteRootPath,
        },
        onSessionIdChange: (sessionId) => {
          remoteAuthSessionIdRef.current = sessionId;
        },
        onAuthorizationUrlChange: setRemoteAuthorizationUrl,
        onMessageChange: setRemoteMessage,
      });
      setRemoteMessage(getRemoteLibraryConnectedMessage(t, provider));
      setStep("stemMode");
    } catch (err: unknown) {
      setError(getErrorMessage(err));
    } finally {
      setLoading(false);
    }
  };

  const handleWebDavConnect = async () => {
    await connectRemoteLibrary("webdav");
  };

  const handleGoogleDriveConnect = async () => {
    await connectRemoteLibrary("google_drive");
  };

  const handleDropboxConnect = async () => {
    await connectRemoteLibrary("dropbox");
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
                                : "Open remote repository",
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
                    "Connect Google Drive, Dropbox, or WebDAV to your remote repository.",
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
                        getRemoteProviderDisplayName(t, choice.provider),
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
                        {t(choice.title, { defaultValue: choice.title })}
                      </div>
                      <div className="text-[12px] text-[var(--color-text-dim)]">
                        {t(choice.description, {
                          defaultValue: choice.description,
                        })}
                      </div>
                      <div className="mt-1 text-[11px] text-[var(--color-text-dimmer)]">
                        {choice.availableNow
                          ? t("setup.remoteProvider.availableNow", {
                              defaultValue: "Available in this build",
                            })
                          : t("setup.remoteProvider.plannedLater", {
                              defaultValue:
                                "Planned in the next provider phase",
                            })}
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
                    {t("settings.library.displayName", {
                      defaultValue: "Display name",
                    })}
                  </label>
                  <input
                    value={remoteDisplayName}
                    onChange={(event) =>
                      setRemoteDisplayName(event.target.value)
                    }
                    placeholder={getRemoteProviderDisplayName(
                      t,
                      "google_drive",
                    )}
                    className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-2 text-[13px] text-white outline-none transition-colors focus:border-[var(--color-accent)]"
                  />
                </div>
                <p className="text-[11px] text-[var(--color-text-dimmer)]">
                  {t("settings.library.googleDriveBundledDescription", {
                    defaultValue:
                      "OpenKara uses its bundled Google Drive app registration and will create or reuse a folder with this display name in My Drive.",
                  })}
                </p>

                <button
                  onClick={() => void handleGoogleDriveConnect()}
                  disabled={loading}
                  className="w-full rounded-lg bg-[var(--color-accent)] px-4 py-2.5 text-[13px] font-medium text-white transition-opacity hover:opacity-90 disabled:opacity-60"
                >
                  {loading
                    ? t("settings.library.waitingForGoogle", {
                        defaultValue: "Waiting for Google…",
                      })
                    : t("settings.library.connectGoogleDrive", {
                        defaultValue: "Connect Google Drive",
                      })}
                </button>

                {remoteAuthorizationUrl && (
                  <a
                    href={remoteAuthorizationUrl}
                    target="_blank"
                    rel="noreferrer"
                    className="block text-[12px] text-[var(--color-accent)] underline underline-offset-2"
                  >
                    {t("settings.library.openGoogleBrowserSignInAgain", {
                      defaultValue: "Open Google sign-in in your browser again",
                    })}
                  </a>
                )}
              </div>
            )}

            {selectedRemoteProvider === "webdav" && (
              <div className="space-y-3 rounded-lg border border-[var(--color-border-light)] bg-[var(--color-sidebar)] p-4 text-left">
                <div>
                  <label className="mb-1 block text-[12px] font-medium text-white">
                    {t("settings.library.displayName", {
                      defaultValue: "Display name",
                    })}
                  </label>
                  <input
                    value={remoteDisplayName}
                    onChange={(event) =>
                      setRemoteDisplayName(event.target.value)
                    }
                    placeholder={getRemoteProviderDisplayName(t, "webdav")}
                    className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-2 text-[13px] text-white outline-none transition-colors focus:border-[var(--color-accent)]"
                  />
                </div>

                <div>
                  <label className="mb-1 block text-[12px] font-medium text-white">
                    {t("settings.library.webdavServerUrl", {
                      defaultValue: "Server URL",
                    })}
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
                    {t("settings.library.webdavLibraryPath", {
                      defaultValue: "Library path",
                    })}
                  </label>
                  <input
                    value={remoteRootPath}
                    onChange={(event) => setRemoteRootPath(event.target.value)}
                    placeholder="/OpenKara"
                    className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-2 text-[13px] text-white outline-none transition-colors focus:border-[var(--color-accent)]"
                    spellCheck={false}
                  />
                  <p className="mt-1 text-[11px] text-[var(--color-text-dimmer)]">
                    {t("settings.library.webdavLibraryPathDescription", {
                      defaultValue:
                        "Point this at an existing remote repository path, or enter a new folder name and OpenKara will initialize it for you.",
                    })}
                  </p>
                </div>

                <div className="grid grid-cols-1 gap-3 sm:grid-cols-2">
                  <div>
                    <label className="mb-1 block text-[12px] font-medium text-white">
                      {t("settings.library.webdavUsername", {
                        defaultValue: "Username",
                      })}
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
                      {t("settings.library.webdavPassword", {
                        defaultValue: "Password",
                      })}
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
                  {loading
                    ? t("settings.library.connecting", {
                        defaultValue: "Connecting…",
                      })
                    : t("settings.library.connectWebdavLibrary", {
                        defaultValue: "Connect WebDAV library",
                      })}
                </button>
              </div>
            )}

            {selectedRemoteProvider === "dropbox" && (
              <div className="space-y-3 rounded-lg border border-[var(--color-border-light)] bg-[var(--color-sidebar)] p-4 text-left">
                <div>
                  <label className="mb-1 block text-[12px] font-medium text-white">
                    {t("settings.library.displayName", {
                      defaultValue: "Display name",
                    })}
                  </label>
                  <input
                    value={remoteDisplayName}
                    onChange={(event) =>
                      setRemoteDisplayName(event.target.value)
                    }
                    placeholder={getRemoteProviderDisplayName(t, "dropbox")}
                    className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-2 text-[13px] text-white outline-none transition-colors focus:border-[var(--color-accent)]"
                  />
                </div>
                <p className="text-[11px] text-[var(--color-text-dimmer)]">
                  {t("settings.library.dropboxBundledDescription", {
                    defaultValue:
                      "OpenKara uses its bundled Dropbox app registration and will create or reuse a folder with this display name in Dropbox.",
                  })}
                </p>

                <button
                  onClick={() => void handleDropboxConnect()}
                  disabled={loading}
                  className="w-full rounded-lg bg-[var(--color-accent)] px-4 py-2.5 text-[13px] font-medium text-white transition-opacity hover:opacity-90 disabled:opacity-60"
                >
                  {loading
                    ? t("settings.library.waitingForDropbox", {
                        defaultValue: "Waiting for Dropbox…",
                      })
                    : t("settings.library.connectDropbox", {
                        defaultValue: "Connect Dropbox",
                      })}
                </button>

                {remoteAuthorizationUrl && (
                  <a
                    href={remoteAuthorizationUrl}
                    target="_blank"
                    rel="noreferrer"
                    className="block text-[12px] text-[var(--color-accent)] underline underline-offset-2"
                  >
                    {t("settings.library.openDropboxBrowserSignInAgain", {
                      defaultValue:
                        "Open Dropbox sign-in in your browser again",
                    })}
                  </a>
                )}
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
                if (remoteAuthSessionIdRef.current) {
                  void api.cancelRemoteAuth(remoteAuthSessionIdRef.current);
                }
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
