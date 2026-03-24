import { PanelLeft, Plus } from "lucide-react";
import { useTranslation } from "react-i18next";
import { Sidebar } from "./Sidebar";
import { ToastContainer } from "./ToastContainer";
import { ImportCdgChoiceDialog } from "@/components/Library/ImportCdgChoiceDialog";
import { ImportButton } from "@/components/Library/ImportButton";
import { Tooltip } from "@/components/Overlay/Tooltip";
import { getShortcutPlatform } from "@/lib/app-shortcuts";
import {
  createWindowShellStyle,
  getNativeWindowShellState,
  type WindowShellState,
  useWindowShellState,
} from "@/lib/window-shell";
import { useLayoutStore } from "@/stores/layout-store";

interface SidebarWebviewAppProps {
  initialWindowShellState?: WindowShellState;
}

export function SidebarWebviewApp({
  initialWindowShellState,
}: SidebarWebviewAppProps = {}) {
  const { t } = useTranslation();
  const platform = getShortcutPlatform();
  const shellState = useWindowShellState(
    initialWindowShellState ??
      (platform === "mac" ? getNativeWindowShellState() : undefined),
    platform,
  );
  const nativeShell = shellState.tier === "mac_native";
  const toggleSidebar = useLayoutStore((s) => s.toggleSidebar);

  const header = nativeShell ? (
    <div
      className="flex h-[var(--window-shell-toolbar-height)] items-center justify-between gap-3 pr-2"
      data-native-sidebar-header="true"
      data-native-sidebar-header-layout="split"
      style={{
        paddingInlineStart: "var(--window-shell-leading-controls-space)",
      }}
    >
      <Tooltip label={t("toolbar.toggleSidebar")}>
        <button
          onClick={toggleSidebar}
          aria-label={t("toolbar.toggleSidebar")}
          className="motion-icon-button rounded-xl p-2 text-[var(--color-control-primary)] hover:bg-white/6"
        >
          <PanelLeft size={16} />
        </button>
      </Tooltip>

      <Tooltip label={t("toolbar.import")}>
        <ImportButton ariaLabel={t("toolbar.import")}>
          <span className="motion-surface flex items-center justify-center rounded-xl p-2 text-[var(--color-control-primary)] hover:bg-white/8 hover:text-white">
            <Plus size={16} />
          </span>
        </ImportButton>
      </Tooltip>
    </div>
  ) : undefined;

  return (
    <div
      className="flex h-screen w-full overflow-hidden font-sans"
      data-window-chrome-platform={shellState.chromeVariant}
      data-window-shell-tier={shellState.tier}
      style={createWindowShellStyle(shellState)}
    >
      <div className="flex h-full w-full">
        <Sidebar header={header} variant={nativeShell ? "native" : "default"} />
      </div>
      <ToastContainer />
      <ImportCdgChoiceDialog />
    </div>
  );
}
