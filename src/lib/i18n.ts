import i18next from "i18next";
import { initReactI18next } from "react-i18next";
import en from "@/locales/en.json";
import zhCN from "@/locales/zh-CN.json";

export const SUPPORTED_LANGUAGES = [
  { code: "en", name: "English" },
  { code: "zh-CN", name: "简体中文" },
] as const;

export function detectSystemLanguage(): string {
  const nav = navigator.language;
  if (SUPPORTED_LANGUAGES.some((l) => l.code === nav)) return nav;
  const base = nav.split("-")[0];
  const match = SUPPORTED_LANGUAGES.find((l) => l.code.startsWith(base));
  return match?.code ?? "en";
}

i18next.use(initReactI18next).init({
  resources: {
    en: { translation: en },
    "zh-CN": { translation: zhCN },
  },
  lng: "en",
  fallbackLng: "en",
  interpolation: {
    escapeValue: false,
  },
});

export default i18next;
