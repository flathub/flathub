import {
  getCurrentWebviewWindow,
  WebviewWindow,
} from "@tauri-apps/api/webviewWindow";
import { availableMonitors } from "@tauri-apps/api/window";

export async function getMonitors() {
  return availableMonitors();
}

export async function openFullscreenPlayer(monitorIndex?: number) {
  // Close existing fullscreen player window if it exists
  const existing = await WebviewWindow.getByLabel("fullscreen-player");
  if (existing) {
    await existing.close();
  }

  const monitors = await availableMonitors();
  // Default to secondary monitor if available, else primary
  const target = monitors[monitorIndex ?? (monitors.length > 1 ? 1 : 0)];

  new WebviewWindow("fullscreen-player", {
    url: "index.html?mode=fullscreen-player",
    title: "OpenKara Player",
    x: target.position.x,
    y: target.position.y,
    width: target.size.width,
    height: target.size.height,
  });
}

export async function closeFullscreenPlayer() {
  try {
    const currentWindow = getCurrentWebviewWindow();
    if (currentWindow.label === "fullscreen-player") {
      await currentWindow.close();
      return;
    }

    const win = await WebviewWindow.getByLabel("fullscreen-player");
    await win?.close();
  } catch (err) {
    console.error("Failed to close fullscreen player:", err);
  }
}
