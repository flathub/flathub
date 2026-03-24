import { MainContentView } from "./MainContentView";
import { ToastContainer } from "./ToastContainer";
import { ImportCdgChoiceDialog } from "@/components/Library/ImportCdgChoiceDialog";
import { getShortcutPlatform } from "@/lib/app-shortcuts";
import {
  createWindowShellStyle,
  getNativeWindowShellState,
  type WindowShellState,
  useWindowShellState,
} from "@/lib/window-shell";
import { useNativeSidebarVisibilitySync } from "@/runtime/native-shell-runtime";

interface MainWebviewAppProps {
  initialWindowShellState?: WindowShellState;
}

export function MainWebviewApp({
  initialWindowShellState,
}: MainWebviewAppProps = {}) {
  const platform = getShortcutPlatform();
  useNativeSidebarVisibilitySync(true);
  const shellState = useWindowShellState(
    initialWindowShellState ??
      (platform === "mac" ? getNativeWindowShellState() : undefined),
    platform,
  );

  return (
    <div
      className="flex h-screen w-full overflow-hidden font-sans"
      data-window-chrome-platform={shellState.chromeVariant}
      data-window-shell-tier={shellState.tier}
      style={createWindowShellStyle(shellState)}
    >
      <MainContentView shellTier={shellState.tier} />
      <ToastContainer />
      <ImportCdgChoiceDialog />
    </div>
  );
}
