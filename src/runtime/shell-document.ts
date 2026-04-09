/** The document shell marker is always full-app; host uses a single webview tree. */
export function applyShellDocumentMarker() {
  const shellMode = "full-app";

  document.documentElement.dataset.appShell = shellMode;
  document.body.dataset.appShell = shellMode;
  document.getElementById("root")?.setAttribute("data-app-shell", shellMode);
}
