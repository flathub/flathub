import { useRef, useCallback } from "react";
import { useTranslation } from "react-i18next";
import { Search } from "lucide-react";
import { useLibraryStore } from "@/stores/library-store";

export function SearchBox() {
  const { t } = useTranslation();
  const searchQuery = useLibraryStore((s) => s.searchQuery);
  const setSearchQuery = useLibraryStore((s) => s.setSearchQuery);
  const timerRef = useRef<ReturnType<typeof setTimeout>>(undefined);

  const handleChange = useCallback(
    (e: React.ChangeEvent<HTMLInputElement>) => {
      const value = e.target.value;
      // Update local display immediately
      useLibraryStore.setState({ searchQuery: value });
      // Debounce the actual search
      clearTimeout(timerRef.current);
      timerRef.current = setTimeout(() => setSearchQuery(value), 200);
    },
    [setSearchQuery],
  );

  return (
    <div
      className="motion-surface relative flex items-center overflow-hidden rounded-[14px] border border-[var(--sidebar-control-border)] bg-[var(--sidebar-control-bg)] shadow-[inset_0_1px_0_rgba(255,255,255,0.04)] focus-within:border-[color-mix(in_srgb,var(--color-accent)_24%,var(--color-border-light))] focus-within:bg-[var(--sidebar-row-overlay-bg)]"
      data-search-visual-variant="unified"
    >
      <Search
        className="absolute left-3 text-[var(--color-text-dim)]"
        size={14}
      />
      <input
        type="text"
        placeholder={t("common.search")}
        aria-label={t("common.search")}
        value={searchQuery}
        onChange={handleChange}
        className="w-full bg-transparent py-2 pl-9 pr-3 text-[14px] text-[var(--color-text)] outline-none placeholder:text-[var(--color-text-dim)]"
      />
    </div>
  );
}
