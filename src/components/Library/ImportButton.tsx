import { type ReactNode } from "react";
import { useLibraryStore } from "@/stores/library-store";
import { promptImportFiles } from "@/runtime/menu-runtime";

interface ImportButtonProps {
  children: ReactNode;
}

export function ImportButton({ children }: ImportButtonProps) {
  const importFiles = useLibraryStore((s) => s.importFiles);

  const handleClick = async () => {
    await promptImportFiles({ importFiles });
  };

  return <button onClick={handleClick}>{children}</button>;
}
