import type { AppSettings } from "@/types/ipc";

interface StartupSettingsDependencies {
  getSettings: () => Promise<AppSettings>;
  hydrateAppSettings: (settings: AppSettings) => void;
  changeLanguage: (language: string) => Promise<unknown>;
  detectFallbackLanguage: () => string;
}

export async function loadStartupSettings({
  getSettings,
  hydrateAppSettings,
  changeLanguage,
  detectFallbackLanguage,
}: StartupSettingsDependencies) {
  const settings = await getSettings();

  // Runtime owns persisted settings hydration so every window reads the same
  // authoritative snapshot instead of racing its own startup fetches.
  hydrateAppSettings(settings);

  const language = settings.language ?? detectFallbackLanguage();
  await changeLanguage(language);
}
