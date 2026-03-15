import { Music } from "lucide-react";
import { useTranslation } from "react-i18next";
import { ImportButton } from "./ImportButton";

export function EmptyLibrary() {
  const { t } = useTranslation();
  return (
    <div className="flex flex-1 flex-col items-center justify-center gap-3 px-4 py-8 text-center">
      <Music size={32} className="text-[var(--color-text-dimmer)]" />
      <p className="text-[12px] text-[var(--color-text-dim)]">{t("library.noTracks")}</p>
      <ImportButton>
        <span className="rounded-md border border-[var(--color-border-light)] bg-[var(--color-hover)] px-3 py-1 text-[12px] text-[var(--color-text)] transition-colors hover:bg-[var(--color-active)] hover:text-white">
          {t("library.importMusic")}
        </span>
      </ImportButton>
    </div>
  );
}
