import { useRef, useCallback } from "react";
import { useTranslation } from "react-i18next";
import { Search } from "lucide-react";
import { useLibraryStore } from "@/stores/library-store";

interface SearchBoxProps {
  variant?: "default" | "native";
}

export function SearchBox({ variant = "default" }: SearchBoxProps = {}) {
  const { t } = useTranslation();
  const searchQuery = useLibraryStore((s) => s.searchQuery);
  const setSearchQuery = useLibraryStore((s) => s.setSearchQuery);
  const timerRef = useRef<ReturnType<typeof setTimeout>>(undefined);
  const nativeVariant = variant === "native";

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
      className={`motion-surface relative flex items-center overflow-hidden border ${
        nativeVariant
          ? "rounded-[14px] border-[var(--native-sidebar-control-border)] bg-[var(--native-sidebar-control-bg)] shadow-[inset_0_1px_0_rgba(255,255,255,0.04)]"
          : "rounded-[8px] border-transparent bg-[var(--color-hover)]"
      } focus-within:border-[color-mix(in_srgb,var(--color-accent)_24%,var(--color-border-light))] ${
        nativeVariant
          ? "focus-within:bg-[var(--native-sidebar-overlay-bg)]"
          : "focus-within:bg-[var(--color-active)]"
      }`}
      data-native-overlay-surface={nativeVariant ? "search-control" : undefined}
      data-search-visual-variant={variant}
    >
      <Search
        className={`absolute text-[var(--color-text-dim)] ${nativeVariant ? "left-3" : "left-2"}`}
        size={14}
      />
      <input
        type="text"
        placeholder={t("common.search")}
        value={searchQuery}
        onChange={handleChange}
        className={`w-full bg-transparent text-[var(--color-text)] outline-none placeholder:text-[var(--color-text-dim)] ${
          nativeVariant
            ? "py-2 pl-9 pr-3 text-[14px]"
            : "py-1.5 pl-7 pr-3 text-[13px]"
        }`}
      />
    </div>
  );
}
