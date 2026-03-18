import { useEffect, useState } from "react";
import { useTranslation } from "react-i18next";
import { open } from "@tauri-apps/plugin-dialog";
import {
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

type Step = "language" | "library" | "stemMode";

interface LibrarySetupProps {
  onComplete: () => void;
}

function StepIndicator({ current }: { current: Step }) {
  const steps: Step[] = ["language", "library", "stemMode"];
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
  const [selectedLanguage, setSelectedLanguage] = useState(
    () =>
      settingsLanguage ?? i18next.resolvedLanguage ?? detectSystemLanguage(),
  );
  const [selectedStemMode, setSelectedStemMode] = useState<
    "two_stem" | "four_stem"
  >(settingsStemMode);

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

    const libraryDir = `${selected}/OpenKara`;
    setLoading(true);
    setError(null);
    try {
      await api.createLibrary(libraryDir);
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

    setLoading(true);
    setError(null);
    try {
      await api.openLibrary(selected);
      setStep("stemMode");
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
              <button
                onClick={handleCreate}
                disabled={loading}
                className="flex w-full items-center gap-3 rounded-lg border border-[var(--color-border-light)] bg-[var(--color-sidebar)] px-5 py-4 text-left transition-colors hover:bg-[var(--color-hover)] disabled:opacity-50"
              >
                <Plus
                  size={20}
                  className="shrink-0 text-[var(--color-accent)]"
                />
                <div>
                  <div className="text-[14px] font-medium text-white">
                    {t("setup.createNew")}
                  </div>
                  <div className="text-[12px] text-[var(--color-text-dim)]">
                    {t("setup.createNewDescription")}
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
                    {t("setup.openExisting")}
                  </div>
                  <div className="text-[12px] text-[var(--color-text-dim)]">
                    {t("setup.openExistingDescription")}
                  </div>
                </div>
              </button>
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
