import { useCallback, useState } from "react";
import { AppLayout } from "@/components/Layout/AppLayout";
import { LibrarySetup } from "@/components/Settings/LibrarySetup";
import { useAppStartupRuntime, useAppRuntime } from "@/runtime/app-runtime";

interface AppProps {
  initialLibraryReady?: boolean | null;
}

function App({ initialLibraryReady = null }: AppProps) {
  const [libraryReady, setLibraryReady] = useState<boolean | null>(
    initialLibraryReady,
  );
  useAppStartupRuntime(libraryReady, setLibraryReady);
  useAppRuntime(libraryReady === true);

  const handleLibrarySetupComplete = useCallback(() => {
    setLibraryReady(true);
  }, []);

  if (libraryReady === null) {
    return null;
  }

  if (!libraryReady) {
    return <LibrarySetup onComplete={handleLibrarySetupComplete} />;
  }

  return <AppLayout />;
}

export default App;
