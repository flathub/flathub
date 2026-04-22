// @vitest-environment jsdom

import { act } from "react";
import { createRoot } from "react-dom/client";
import { renderToStaticMarkup } from "react-dom/server";
import { beforeEach, describe, expect, test, vi } from "vitest";
import { RemoteLibraryWizard } from "./RemoteLibraryWizard";
import {
  SettingsOverlayContext,
  createSettingsOverlayTestContextValue,
} from "./SettingsOverlay.context";

const {
  mockBeginRemoteAuth,
  mockCancelRemoteAuth,
  mockPollRemoteAuth,
  mockCreateRemoteLibrary,
  mockOpenExternalUrl,
  mockRegisterRemoteLibrary,
} = vi.hoisted(() => ({
  mockBeginRemoteAuth: vi.fn(),
  mockCancelRemoteAuth: vi.fn(),
  mockPollRemoteAuth: vi.fn(),
  mockCreateRemoteLibrary: vi.fn(),
  mockOpenExternalUrl: vi.fn(),
  mockRegisterRemoteLibrary: vi.fn(),
}));

vi.mock("react-i18next", () => ({
  initReactI18next: {
    type: "3rdParty",
    init: () => {},
  },
  useTranslation: () => ({
    t: (key: string, options?: Record<string, unknown>) => {
      if (key === "settings.library.mirrorActiveLocalDescriptionWithName") {
        return `settings.library.mirrorActiveLocalDescriptionWithName:${String(options?.displayName ?? "")}`;
      }

      return key;
    },
  }),
}));

vi.mock("@/lib/tauri", () => ({
  beginRemoteAuth: mockBeginRemoteAuth,
  cancelRemoteAuth: mockCancelRemoteAuth,
  pollRemoteAuth: mockPollRemoteAuth,
  createRemoteLibrary: mockCreateRemoteLibrary,
  openExternalUrl: mockOpenExternalUrl,
  registerRemoteLibrary: mockRegisterRemoteLibrary,
}));

async function flushEffects() {
  await act(async () => {
    await Promise.resolve();
    await Promise.resolve();
    await new Promise((resolve) => window.setTimeout(resolve, 0));
  });
}

describe("RemoteLibraryWizard", () => {
  beforeEach(() => {
    mockBeginRemoteAuth.mockReset();
    mockCancelRemoteAuth.mockReset();
    mockPollRemoteAuth.mockReset();
    mockCreateRemoteLibrary.mockReset();
    mockOpenExternalUrl.mockReset();
    mockRegisterRemoteLibrary.mockReset();
    vi.restoreAllMocks();

    (
      globalThis as typeof globalThis & { IS_REACT_ACT_ENVIRONMENT?: boolean }
    ).IS_REACT_ACT_ENVIRONMENT = true;
  });

  test("uses translation keys instead of hardcoded remote-library English copy", () => {
    const value = createSettingsOverlayTestContextValue();

    const markup = renderToStaticMarkup(
      <SettingsOverlayContext value={value}>
        <RemoteLibraryWizard onClose={() => {}} />
      </SettingsOverlayContext>,
    );

    expect(markup).toContain("settings.library.openRemoteLibrary");
    expect(markup).toContain("settings.library.displayName");
    expect(markup).not.toContain("Open Remote Library");
    expect(markup).not.toContain("Display name");
  });

  test("shows structured command error messages instead of [object Object]", async () => {
    mockBeginRemoteAuth.mockResolvedValue({
      session_id: "session-1",
      provider: "google_drive",
      authorization_url: null,
      expires_at_ms: null,
    });
    mockCreateRemoteLibrary.mockRejectedValue({
      code: "internal",
      message: "Google Drive folder creation failed.",
      retryable: false,
    });

    const value = createSettingsOverlayTestContextValue({
      meta: { isInitializing: false },
    });
    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);

    await act(async () => {
      root.render(
        <SettingsOverlayContext value={value}>
          <RemoteLibraryWizard onClose={() => {}} />
        </SettingsOverlayContext>,
      );
    });

    const openRemoteButtons = [...container.querySelectorAll("button")].filter(
      (button) =>
        button.textContent?.includes("settings.library.openRemoteLibrary"),
    );
    const connectButton = openRemoteButtons[openRemoteButtons.length - 1];
    expect(connectButton).toBeTruthy();

    await act(async () => {
      connectButton?.click();
    });
    await flushEffects();

    expect(container.textContent).toContain(
      "Google Drive folder creation failed.",
    );
    expect(container.textContent).not.toContain("[object Object]");

    await act(async () => {
      root.unmount();
    });
    container.remove();
  });

  test("keeps the close button enabled while remote auth is pending", async () => {
    let resolveAuth!: (value: unknown) => void;
    mockBeginRemoteAuth.mockImplementation(
      () =>
        new Promise((resolve) => {
          resolveAuth = resolve;
        }),
    );

    const value = createSettingsOverlayTestContextValue({
      meta: { isInitializing: false },
    });
    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);

    await act(async () => {
      root.render(
        <SettingsOverlayContext value={value}>
          <RemoteLibraryWizard onClose={() => {}} />
        </SettingsOverlayContext>,
      );
    });

    const buttons = [...container.querySelectorAll("button")];
    const closeButton = buttons[0];
    const openRemoteButtons = buttons.filter((button) =>
      button.textContent?.includes("settings.library.openRemoteLibrary"),
    );
    const connectButton = openRemoteButtons[openRemoteButtons.length - 1];
    expect(closeButton).toBeTruthy();
    expect(connectButton).toBeTruthy();

    await act(async () => {
      connectButton?.click();
      await Promise.resolve();
    });

    expect(closeButton?.hasAttribute("disabled")).toBe(false);

    if (resolveAuth) {
      await act(async () => {
        resolveAuth({
          session_id: "session-1",
          provider: "google_drive",
          authorization_url: null,
          expires_at_ms: null,
        });
      });
    }

    await act(async () => {
      root.unmount();
    });
    container.remove();
  });

  test("cancels pending auth when the wizard closes", async () => {
    mockBeginRemoteAuth.mockResolvedValue({
      session_id: "session-1",
      provider: "dropbox",
      authorization_url: "https://example.com/oauth",
      expires_at_ms: null,
    });
    mockOpenExternalUrl.mockResolvedValue(undefined);

    const onClose = vi.fn();
    const value = createSettingsOverlayTestContextValue({
      meta: { isInitializing: false },
    });
    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);

    await act(async () => {
      root.render(
        <SettingsOverlayContext value={value}>
          <RemoteLibraryWizard onClose={onClose} />
        </SettingsOverlayContext>,
      );
    });

    const buttons = [...container.querySelectorAll("button")];
    const closeButton = buttons[0];
    const openRemoteButtons = buttons.filter((button) =>
      button.textContent?.includes("settings.library.openRemoteLibrary"),
    );
    const connectButton = openRemoteButtons[openRemoteButtons.length - 1];

    await act(async () => {
      connectButton?.click();
      await Promise.resolve();
      await Promise.resolve();
    });

    await act(async () => {
      closeButton?.click();
      await Promise.resolve();
    });

    expect(mockCancelRemoteAuth).toHaveBeenCalledWith("session-1");
    expect(onClose).toHaveBeenCalled();

    await act(async () => {
      root.unmount();
    });
    container.remove();
  });

  test("opens OAuth URLs through the dedicated desktop opener", async () => {
    mockBeginRemoteAuth.mockResolvedValue({
      session_id: "session-1",
      provider: "dropbox",
      authorization_url: "https://example.com/oauth",
      expires_at_ms: null,
    });
    mockOpenExternalUrl.mockResolvedValue(undefined);
    mockPollRemoteAuth.mockResolvedValue({
      session_id: "session-1",
      provider: "dropbox",
      state: "failed",
      remote_root_locator: null,
      display_name: null,
      error: { code: "internal", message: "stop", retryable: false },
    });

    const value = createSettingsOverlayTestContextValue({
      meta: { isInitializing: false },
    });
    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);

    await act(async () => {
      root.render(
        <SettingsOverlayContext value={value}>
          <RemoteLibraryWizard onClose={() => {}} />
        </SettingsOverlayContext>,
      );
    });

    const openRemoteButtons = [...container.querySelectorAll("button")].filter(
      (button) =>
        button.textContent?.includes("settings.library.openRemoteLibrary"),
    );
    const connectButton = openRemoteButtons[openRemoteButtons.length - 1];

    await act(async () => {
      connectButton?.click();
    });
    await flushEffects();

    expect(mockOpenExternalUrl).toHaveBeenCalledWith(
      "https://example.com/oauth",
    );

    await act(async () => {
      root.unmount();
    });
    container.remove();
  });

  test("cancels auth if opening the external browser fails", async () => {
    mockBeginRemoteAuth.mockResolvedValue({
      session_id: "session-1",
      provider: "dropbox",
      authorization_url: "https://example.com/oauth",
      expires_at_ms: null,
    });
    mockOpenExternalUrl.mockRejectedValue(new Error("browser open failed"));

    const value = createSettingsOverlayTestContextValue({
      meta: { isInitializing: false },
    });
    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);

    await act(async () => {
      root.render(
        <SettingsOverlayContext value={value}>
          <RemoteLibraryWizard onClose={() => {}} />
        </SettingsOverlayContext>,
      );
    });

    const openRemoteButtons = [...container.querySelectorAll("button")].filter(
      (button) =>
        button.textContent?.includes("settings.library.openRemoteLibrary"),
    );
    const connectButton = openRemoteButtons[openRemoteButtons.length - 1];

    await act(async () => {
      connectButton?.click();
    });
    await flushEffects();

    expect(mockCancelRemoteAuth).toHaveBeenCalledWith("session-1");
    expect(container.textContent).toContain("browser open failed");

    await act(async () => {
      root.unmount();
    });
    container.remove();
  });
});
