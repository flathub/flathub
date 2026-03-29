import { resolveCurrentAppShellMode } from "@/lib/app-shell";

export function applyShellDocumentMarker() {
  const shellMode = resolveCurrentAppShellMode();

  document.documentElement.dataset.appShell = shellMode;
  document.body.dataset.appShell = shellMode;
  document.getElementById("root")?.setAttribute("data-app-shell", shellMode);
}
