import { type ReactNode } from "react";
import { useLibraryStore } from "@/stores/library-store";
import { promptImportFiles } from "@/runtime/menu-runtime";

interface ImportButtonProps {
  children: ReactNode;
  ariaLabel?: string;
}

export function ImportButton({ children, ariaLabel }: ImportButtonProps) {
  const importFiles = useLibraryStore((s) => s.importFiles);

  const handleClick = async () => {
    await promptImportFiles({ importFiles });
  };

  return (
    <button onClick={handleClick} aria-label={ariaLabel}>
      {children}
    </button>
  );
}
