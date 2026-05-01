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
  mockResolveRemoteLibraryCandidate,
  mockOpenExternalUrl,
  mockRegisterRemoteLibrary,
  mockReauthorizeRemoteLibrary,
} = vi.hoisted(() => ({
  mockBeginRemoteAuth: vi.fn(),
  mockCancelRemoteAuth: vi.fn(),
  mockPollRemoteAuth: vi.fn(),
  mockCreateRemoteLibrary: vi.fn(),
  mockResolveRemoteLibraryCandidate: vi.fn(),
  mockOpenExternalUrl: vi.fn(),
  mockRegisterRemoteLibrary: vi.fn(),
  mockReauthorizeRemoteLibrary: vi.fn(),
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
  resolveRemoteLibraryCandidate: mockResolveRemoteLibraryCandidate,
  openExternalUrl: mockOpenExternalUrl,
  registerRemoteLibrary: mockRegisterRemoteLibrary,
  reauthorizeRemoteLibrary: mockReauthorizeRemoteLibrary,
}));

async function flushEffects() {
  await act(async () => {
    await Promise.resolve();
    await Promise.resolve();
    await new Promise((resolve) => window.setTimeout(resolve, 0));
  });
}

function setInputValue(input: HTMLInputElement, value: string) {
  const setter = Object.getOwnPropertyDescriptor(
    HTMLInputElement.prototype,
    "value",
  )?.set;
  setter?.call(input, value);
  input.dispatchEvent(new Event("input", { bubbles: true }));
}

describe("RemoteLibraryWizard", () => {
  beforeEach(() => {
    mockBeginRemoteAuth.mockReset();
    mockCancelRemoteAuth.mockReset();
    mockPollRemoteAuth.mockReset();
    mockCreateRemoteLibrary.mockReset();
    mockResolveRemoteLibraryCandidate.mockReset();
    mockOpenExternalUrl.mockReset();
    mockRegisterRemoteLibrary.mockReset();
    mockReauthorizeRemoteLibrary.mockReset();
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

  test("renders reauthorization copy and preselects the requested provider", () => {
    const value = createSettingsOverlayTestContextValue();

    const markup = renderToStaticMarkup(
      <SettingsOverlayContext value={value}>
        <RemoteLibraryWizard
          onClose={() => {}}
          initialProvider="webdav"
          purpose="reauthorize"
        />
      </SettingsOverlayContext>,
    );

    expect(markup).toContain("settings.library.reauthorizeRemoteRepository");
    expect(markup).toContain(
      "settings.library.reauthorizeRemoteRepositoryDescription",
    );
    expect(markup).toContain("settings.library.webdavServerUrl");
  });

  test("reauthorizes an existing remote repository without registering a new one when the location is unchanged", async () => {
    mockBeginRemoteAuth.mockResolvedValue({
      session_id: "session-1",
      provider: "webdav",
      authorization_url: null,
      expires_at_ms: null,
    });
    mockResolveRemoteLibraryCandidate.mockResolvedValue({
      provider: "webdav",
      remote_root_locator: "https://dav.example.com/OpenKara/",
      remote_path_display: "dav.example.com/OpenKara",
      display_name: "Drive",
      account_id: "user@dav.example.com",
    });
    mockReauthorizeRemoteLibrary.mockResolvedValue({
      active_library_id: "remote:existing",
      libraries: [],
    });
    const switchLibrary = vi.fn();
    const onClose = vi.fn();
    const value = createSettingsOverlayTestContextValue(
      {
        meta: { isInitializing: false },
        state: {
          libraries: [
            {
              id: "remote:existing",
              kind: "remote",
              display_name: "Drive",
              provider: "webdav",
              remote_root_locator: "https://dav.example.com/OpenKara/",
              remote_path_display: "dav.example.com/OpenKara",
              account_id: "user@dav.example.com",
              connection_config: {
                type: "webdav",
                server_url: "https://dav.example.com/",
              },
              cached_db_path: "/tmp/openkara.db",
              remote_revision: "rev-1",
            },
          ],
          activeLibraryId: "remote:existing",
        },
      },
      { switchLibrary },
    );
    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);

    await act(async () => {
      root.render(
        <SettingsOverlayContext value={value}>
          <RemoteLibraryWizard
            onClose={onClose}
            libraryId="remote:existing"
            initialProvider="webdav"
            initialDisplayName="Drive"
            initialServerUrl="https://dav.example.com/"
            initialRemoteRootLocator="https://dav.example.com/OpenKara/"
            initialRemotePathDisplay="dav.example.com/OpenKara"
            initialRootPath="/OpenKara"
            purpose="reauthorize"
          />
        </SettingsOverlayContext>,
      );
    });

    const inputs = [...container.querySelectorAll("input")];
    await act(async () => {
      setInputValue(inputs[3], "user");
      setInputValue(inputs[4], "secret");
    });

    const reauthorizeButton = [...container.querySelectorAll("button")].find(
      (button) =>
        button.textContent?.includes(
          "settings.library.reauthorizeRemoteRepository",
        ),
    );
    expect(reauthorizeButton).toBeTruthy();

    await act(async () => {
      reauthorizeButton?.click();
    });
    await flushEffects();

    expect(mockReauthorizeRemoteLibrary).toHaveBeenCalledWith(
      "remote:existing",
      "session-1",
      "https://dav.example.com/OpenKara/",
      "Drive",
      false,
    );
    expect(mockRegisterRemoteLibrary).not.toHaveBeenCalled();
    expect(switchLibrary).toHaveBeenCalledWith("remote:existing");
    expect(onClose).toHaveBeenCalled();

    await act(async () => {
      root.unmount();
    });
    container.remove();
  });

  test("asks before relocating a remote repository when reauthorization returns a different location", async () => {
    mockBeginRemoteAuth.mockResolvedValue({
      session_id: "session-1",
      provider: "webdav",
      authorization_url: null,
      expires_at_ms: null,
    });
    mockResolveRemoteLibraryCandidate.mockResolvedValue({
      provider: "webdav",
      remote_root_locator: "https://dav.example.com/MovedOpenKara/",
      remote_path_display: "dav.example.com/MovedOpenKara",
      display_name: "Drive",
      account_id: "user@dav.example.com",
    });
    mockReauthorizeRemoteLibrary.mockResolvedValue({
      active_library_id: "remote:existing",
      libraries: [],
    });
    const confirm = vi.spyOn(window, "confirm").mockReturnValue(true);
    const value = createSettingsOverlayTestContextValue({
      meta: { isInitializing: false },
    });
    const container = document.createElement("div");
    document.body.appendChild(container);
    const root = createRoot(container);

    await act(async () => {
      root.render(
        <SettingsOverlayContext value={value}>
          <RemoteLibraryWizard
            onClose={() => {}}
            libraryId="remote:existing"
            initialProvider="webdav"
            initialDisplayName="Drive"
            initialServerUrl="https://dav.example.com/"
            initialRemoteRootLocator="https://dav.example.com/OpenKara/"
            initialRemotePathDisplay="dav.example.com/OpenKara"
            initialRootPath="/OpenKara"
            purpose="reauthorize"
          />
        </SettingsOverlayContext>,
      );
    });

    const inputs = [...container.querySelectorAll("input")];
    await act(async () => {
      setInputValue(inputs[3], "user");
      setInputValue(inputs[4], "secret");
    });

    const reauthorizeButton = [...container.querySelectorAll("button")].find(
      (button) =>
        button.textContent?.includes(
          "settings.library.reauthorizeRemoteRepository",
        ),
    );

    await act(async () => {
      reauthorizeButton?.click();
    });
    await flushEffects();

    expect(confirm).toHaveBeenCalledWith(
      expect.stringContaining(
        "settings.library.confirmRemoteRepositoryRelocation",
      ),
    );
    expect(mockReauthorizeRemoteLibrary).toHaveBeenCalledWith(
      "remote:existing",
      "session-1",
      "https://dav.example.com/MovedOpenKara/",
      "Drive",
      true,
    );

    await act(async () => {
      root.unmount();
    });
    container.remove();
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

  test("cancels pending auth when the wizard unmounts", async () => {
    mockBeginRemoteAuth.mockResolvedValue({
      session_id: "session-1",
      provider: "dropbox",
      authorization_url: "https://example.com/oauth",
      expires_at_ms: null,
    });
    mockOpenExternalUrl.mockResolvedValue(undefined);

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
      await Promise.resolve();
      await Promise.resolve();
    });

    await act(async () => {
      root.unmount();
      await Promise.resolve();
    });

    expect(mockCancelRemoteAuth).toHaveBeenCalledWith("session-1");
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
