import { useCallback, useState } from "react";
import { AppLayout } from "@/components/Layout/AppLayout";
import { LibrarySetup } from "@/components/Settings/LibrarySetup";
import {
  useAppStartupRuntime,
  useMainWindowRuntimeWhen,
} from "@/runtime/app-runtime";

function App() {
  const [libraryReady, setLibraryReady] = useState<boolean | null>(null);
  useAppStartupRuntime(libraryReady, setLibraryReady);
  useMainWindowRuntimeWhen(libraryReady === true);

  const handleLibrarySetupComplete = useCallback(() => {
    setLibraryReady(true);
  }, []);

  // Show nothing while checking library state
  if (libraryReady === null) {
    return null;
  }

  // Show setup wizard if no library is configured
  if (!libraryReady) {
    return <LibrarySetup onComplete={handleLibrarySetupComplete} />;
  }

  return <AppLayout />;
}
export default App;
