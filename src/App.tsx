import { useCallback, useState } from "react";
import { AppLayout } from "@/components/Layout/AppLayout";
import { MainWebviewApp } from "@/components/Layout/MainWebviewApp";
import { SidebarWebviewApp } from "@/components/Layout/SidebarWebviewApp";
import { LibrarySetup } from "@/components/Settings/LibrarySetup";
import { resolveCurrentAppShellMode, type AppShellMode } from "@/lib/app-shell";
import {
  useAppStartupRuntime,
  useMainWindowRuntimeWhen,
  useSidebarWindowRuntimeWhen,
} from "@/runtime/app-runtime";

interface AppProps {
  initialLibraryReady?: boolean | null;
  shellMode?: AppShellMode;
}

function App({
  initialLibraryReady = null,
  shellMode = resolveCurrentAppShellMode(),
}: AppProps) {
  const [libraryReady, setLibraryReady] = useState<boolean | null>(
    initialLibraryReady,
  );
  useAppStartupRuntime(libraryReady, setLibraryReady);
  useMainWindowRuntimeWhen(
    libraryReady === true && shellMode !== "sidebar-webview",
  );
  useSidebarWindowRuntimeWhen(
    libraryReady === true && shellMode === "sidebar-webview",
  );

  const handleLibrarySetupComplete = useCallback(() => {
    setLibraryReady(true);
  }, []);

  // Show nothing while checking library state
  if (libraryReady === null) {
    if (
      shellMode === "sidebar-webview" ||
      shellMode === "main-content-webview"
    ) {
      switch (shellMode) {
        case "sidebar-webview":
          return <SidebarWebviewApp />;
        case "main-content-webview":
          return <MainWebviewApp />;
      }
    }

    return null;
  }

  // Show setup wizard if no library is configured
  if (!libraryReady) {
    if (shellMode === "sidebar-webview") {
      return null;
    }

    return <LibrarySetup onComplete={handleLibrarySetupComplete} />;
  }

  switch (shellMode) {
    case "sidebar-webview":
      return <SidebarWebviewApp />;
    case "main-content-webview":
      return <MainWebviewApp />;
    default:
      return <AppLayout />;
  }
}
export default App;
