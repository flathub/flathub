import { useEffect, useState } from "react";
import { useTranslation } from "react-i18next";
import { open } from "@tauri-apps/plugin-dialog";
import {
  type LucideIcon,
  Cloud,
  FolderOpen,
  Server,
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
import type {
  RemoteLibraryProvider,
  RemoteWebDavAuthPayload,
} from "@/types/ipc";

type Step = "language" | "library" | "remoteProvider" | "stemMode";
type LibraryChoiceKind = "create_local" | "open_local" | "open_remote";

interface RemoteProviderChoice {
  provider: RemoteLibraryProvider;
  icon: LucideIcon;
  title: string;
  description: string;
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
    title: "Google Drive",
    description: "Sign in with your Google account in the browser.",
  },
  {
    provider: "dropbox",
    icon: Globe,
    title: "Dropbox",
    description: "Sign in with your Dropbox account in the browser.",
  },
  {
    provider: "webdav",
    icon: Server,
    title: "WebDAV",
    description: "Connect with a server URL and credentials.",
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
  const [webDavBaseUrl, setWebDavBaseUrl] = useState("");
  const [webDavUsername, setWebDavUsername] = useState("");
  const [webDavPassword, setWebDavPassword] = useState("");
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
    setWebDavBaseUrl("");
    setWebDavUsername("");
    setWebDavPassword("");
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

  const handleRemoteProviderSelect = async (
    provider: RemoteLibraryProvider,
  ) => {
    setError(null);
    setRemoteMessage(null);
    setSelectedRemoteProvider(provider);

    if (provider === "webdav") {
      return;
    }

    setLoading(true);
    try {
      const start = await api.beginRemoteAuth(provider);
      setRemoteMessage(
        start.authorization_url
          ? t("setup.remoteAuthBrowserPrompt", {
              defaultValue:
                "Sign-in has opened in your browser. Finish the provider flow there.",
            })
          : t("setup.remoteAuthStarted", {
              defaultValue: "Remote authorization has started.",
            }),
      );
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : String(err);
      setError(message);
    } finally {
      setLoading(false);
    }
  };

  const handleConnectWebDav = async () => {
    setError(null);
    setRemoteMessage(null);
    setLoading(true);

    try {
      const payload: RemoteWebDavAuthPayload = {
        base_url: webDavBaseUrl.trim(),
        username: webDavUsername.trim(),
        password: webDavPassword,
      };

      const start = await api.beginRemoteAuth("webdav", payload);
      setRemoteMessage(
        start.authorization_url
          ? t("setup.remoteAuthBrowserPrompt", {
              defaultValue:
                "WebDAV credentials were accepted. Continue in the browser if prompted.",
            })
          : t("setup.remoteAuthStarted", {
              defaultValue: "WebDAV connection has started.",
            }),
      );
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : String(err);
      setError(message);
    } finally {
      setLoading(false);
    }
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
                    "Google Drive and Dropbox open the browser for OAuth. WebDAV uses server credentials directly.",
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
                    onClick={() =>
                      void handleRemoteProviderSelect(choice.provider)
                    }
                    disabled={loading}
                    className={`flex w-full items-start gap-3 rounded-lg border px-5 py-4 text-left transition-colors disabled:opacity-50 ${
                      isActive
                        ? "border-[var(--color-accent)] bg-[var(--color-accent)]/10"
                        : "border-[var(--color-border-light)] bg-[var(--color-sidebar)] hover:bg-[var(--color-hover)]"
                    }`}
                  >
                    <Icon
                      size={20}
                      className={`mt-0.5 shrink-0 ${
                        choice.provider === "webdav"
                          ? "text-[var(--color-text-dim)]"
                          : "text-[var(--color-accent)]"
                      }`}
                    />
                    <div className="min-w-0 flex-1">
                      <div className="text-[14px] font-medium text-white">
                        {choice.title}
                      </div>
                      <div className="text-[12px] text-[var(--color-text-dim)]">
                        {choice.description}
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

            {selectedRemoteProvider === "webdav" && (
              <div className="space-y-3 rounded-lg border border-[var(--color-border-light)] bg-[var(--color-sidebar)] p-4 text-left">
                <div className="space-y-1">
                  <p className="text-[13px] font-medium text-white">
                    {t("setup.webdavTitle", {
                      defaultValue: "WebDAV connection",
                    })}
                  </p>
                  <p className="text-[12px] text-[var(--color-text-dim)]">
                    {t("setup.webdavDescription", {
                      defaultValue:
                        "Enter the server URL and a username/password or app password.",
                    })}
                  </p>
                </div>
                <label className="block space-y-1">
                  <span className="text-[11px] uppercase text-[var(--color-text-dim)]">
                    {t("setup.webdavUrl", { defaultValue: "Server URL" })}
                  </span>
                  <input
                    value={webDavBaseUrl}
                    onChange={(event) => setWebDavBaseUrl(event.target.value)}
                    placeholder="https://example.com/remote.php/dav/files/user"
                    className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-2 text-[13px] text-white outline-none focus:border-[var(--color-accent)]"
                  />
                </label>
                <label className="block space-y-1">
                  <span className="text-[11px] uppercase text-[var(--color-text-dim)]">
                    {t("setup.webdavUsername", { defaultValue: "Username" })}
                  </span>
                  <input
                    value={webDavUsername}
                    onChange={(event) => setWebDavUsername(event.target.value)}
                    className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-2 text-[13px] text-white outline-none focus:border-[var(--color-accent)]"
                  />
                </label>
                <label className="block space-y-1">
                  <span className="text-[11px] uppercase text-[var(--color-text-dim)]">
                    {t("setup.webdavPassword", { defaultValue: "Password" })}
                  </span>
                  <input
                    type="password"
                    value={webDavPassword}
                    onChange={(event) => setWebDavPassword(event.target.value)}
                    className="w-full rounded-md border border-[var(--color-border-light)] bg-[var(--color-surface)] px-3 py-2 text-[13px] text-white outline-none focus:border-[var(--color-accent)]"
                  />
                </label>
                <button
                  onClick={() => void handleConnectWebDav()}
                  disabled={
                    loading ||
                    webDavBaseUrl.trim().length === 0 ||
                    webDavUsername.trim().length === 0 ||
                    webDavPassword.length === 0
                  }
                  className="w-full rounded-lg bg-[var(--color-accent)] px-5 py-3 text-[14px] font-medium text-white transition-opacity hover:opacity-90 disabled:opacity-50"
                >
                  {loading
                    ? t("setup.settingUp")
                    : t("setup.connectWebdav", {
                        defaultValue: "Connect WebDAV",
                      })}
                </button>
              </div>
            )}

            {selectedRemoteProvider && selectedRemoteProvider !== "webdav" && (
              <p className="text-[13px] text-[var(--color-text-dim)]">
                {remoteMessage ??
                  t("setup.remoteAuthBrowserPrompt", {
                    defaultValue:
                      "Sign-in has been started in your browser. Return here after the provider flow completes.",
                  })}
              </p>
            )}

            {selectedRemoteProvider === "webdav" && remoteMessage && (
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
