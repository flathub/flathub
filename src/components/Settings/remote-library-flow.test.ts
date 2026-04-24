// @vitest-environment jsdom

import type { TFunction } from "i18next";
import { beforeEach, describe, expect, test, vi } from "vitest";
import {
  REMOTE_AUTH_CANCELLED,
  REMOTE_AUTH_POLL_INTERVAL_MS,
  REMOTE_AUTH_TIMEOUT_MS,
  pollRemoteAuthUntilReady,
  runRemoteLibraryRegistrationFlow,
  validateWebDavRemoteLibraryFields,
  type RemoteLibraryFlowApi,
} from "./remote-library-flow";
import { getRemoteProviderDisplayName } from "./remote-library-copy";

const t = ((key: string) => key) as TFunction;
const tWithDefault = ((key: string, options?: { defaultValue?: string }) =>
  options?.defaultValue ?? key) as TFunction;

function createRemoteApiMock(): RemoteLibraryFlowApi {
  return {
    beginRemoteAuth: vi.fn(),
    openExternalUrl: vi.fn(),
    pollRemoteAuth: vi.fn(),
    createRemoteLibrary: vi.fn(),
    registerRemoteLibrary: vi.fn(),
    cancelRemoteAuth: vi.fn(),
  };
}

describe("remote-library-flow", () => {
  beforeEach(() => {
    vi.restoreAllMocks();
  });

  test("validates the required WebDAV fields in order", () => {
    expect(
      validateWebDavRemoteLibraryFields(t, {
        serverUrl: "",
        username: "user",
        password: "secret",
      }),
    ).toBe("settings.library.webdavEnterServerUrl");
    expect(
      validateWebDavRemoteLibraryFields(t, {
        serverUrl: "https://dav.example.com",
        username: "",
        password: "secret",
      }),
    ).toBe("settings.library.webdavEnterUsername");
    expect(
      validateWebDavRemoteLibraryFields(t, {
        serverUrl: "https://dav.example.com",
        username: "user",
        password: "",
      }),
    ).toBe("settings.library.webdavEnterPassword");
    expect(
      validateWebDavRemoteLibraryFields(t, {
        serverUrl: "https://dav.example.com",
        username: "user",
        password: "secret",
      }),
    ).toBeNull();
  });

  test("uses OpenKara as the default remote library display name for every provider", () => {
    expect(getRemoteProviderDisplayName(tWithDefault, "google_drive")).toBe(
      "OpenKara",
    );
    expect(getRemoteProviderDisplayName(tWithDefault, "dropbox")).toBe(
      "OpenKara",
    );
    expect(getRemoteProviderDisplayName(tWithDefault, "webdav")).toBe(
      "OpenKara",
    );
  });

  test("polls remote auth until the session becomes ready", async () => {
    const remoteApi = {
      pollRemoteAuth: vi
        .fn()
        .mockResolvedValueOnce({
          session_id: "session-1",
          provider: "dropbox",
          state: "pending",
          remote_root_locator: null,
          display_name: null,
          error: null,
        })
        .mockResolvedValueOnce({
          session_id: "session-1",
          provider: "dropbox",
          state: "ready",
          remote_root_locator: "/OpenKara",
          display_name: "Dropbox Library",
          error: null,
        }),
    };
    let currentTime = 0;

    await pollRemoteAuthUntilReady({
      sessionId: "session-1",
      provider: "dropbox",
      t,
      remoteApi,
      now: () => currentTime,
      wait: async (durationMs) => {
        currentTime += durationMs;
      },
    });

    expect(remoteApi.pollRemoteAuth).toHaveBeenCalledTimes(2);
  });

  test("times out browser auth with the provider-specific message", async () => {
    const remoteApi = {
      pollRemoteAuth: vi.fn().mockResolvedValue({
        session_id: "session-1",
        provider: "google_drive",
        state: "pending",
        remote_root_locator: null,
        display_name: null,
        error: null,
      }),
    };
    let currentTime = 0;

    await expect(
      pollRemoteAuthUntilReady({
        sessionId: "session-1",
        provider: "google_drive",
        t,
        remoteApi,
        now: () => currentTime,
        wait: async (durationMs) => {
          currentTime += durationMs;
        },
      }),
    ).rejects.toThrow("settings.library.googleSignInTimedOut");

    expect(currentTime).toBe(REMOTE_AUTH_TIMEOUT_MS);
    expect(remoteApi.pollRemoteAuth).toHaveBeenCalledTimes(
      REMOTE_AUTH_TIMEOUT_MS / REMOTE_AUTH_POLL_INTERVAL_MS,
    );
  });

  test("propagates the shared cancelled sentinel when auth is cancelled", async () => {
    await expect(
      pollRemoteAuthUntilReady({
        sessionId: "session-1",
        provider: "dropbox",
        t,
        remoteApi: {
          pollRemoteAuth: vi.fn(),
        },
        isCancelled: () => true,
      }),
    ).rejects.toThrow(REMOTE_AUTH_CANCELLED);
  });

  test("runs the shared auth and registration sequence and cancels on failure", async () => {
    const remoteApi = createRemoteApiMock();
    vi.mocked(remoteApi.beginRemoteAuth).mockResolvedValue({
      session_id: "session-1",
      provider: "dropbox",
      authorization_url: "https://example.com/oauth",
      expires_at_ms: null,
    });
    vi.mocked(remoteApi.openExternalUrl).mockResolvedValue(undefined);
    vi.mocked(remoteApi.pollRemoteAuth).mockResolvedValue({
      session_id: "session-1",
      provider: "dropbox",
      state: "ready",
      remote_root_locator: "/OpenKara",
      display_name: "Dropbox Library",
      error: null,
    });
    vi.mocked(remoteApi.createRemoteLibrary).mockResolvedValue({
      provider: "dropbox",
      remote_root_locator: "/OpenKara",
      remote_path_display: "/OpenKara",
      display_name: "Dropbox Library",
      account_id: "account-1",
    });
    vi.mocked(remoteApi.registerRemoteLibrary).mockRejectedValue(
      new Error("registration failed"),
    );
    const sessionIds: Array<string | null> = [];
    const messages: Array<string | null> = [];
    const authorizationUrls: Array<string | null> = [];

    await expect(
      runRemoteLibraryRegistrationFlow({
        provider: "dropbox",
        displayName: "Dropbox Library",
        t,
        remoteApi,
        onSessionIdChange: (sessionId) => {
          sessionIds.push(sessionId);
        },
        onAuthorizationUrlChange: (authorizationUrl) => {
          authorizationUrls.push(authorizationUrl);
        },
        onMessageChange: (message) => {
          messages.push(message);
        },
      }),
    ).rejects.toThrow("registration failed");

    expect(remoteApi.beginRemoteAuth).toHaveBeenCalledWith("dropbox", null);
    expect(remoteApi.openExternalUrl).toHaveBeenCalledWith(
      "https://example.com/oauth",
    );
    expect(remoteApi.createRemoteLibrary).toHaveBeenCalledWith(
      "session-1",
      "Dropbox Library",
    );
    expect(remoteApi.registerRemoteLibrary).toHaveBeenCalledWith(
      "session-1",
      "/OpenKara",
      "Dropbox Library",
    );
    expect(remoteApi.cancelRemoteAuth).toHaveBeenCalledWith("session-1");
    expect(sessionIds).toEqual(["session-1", null]);
    expect(authorizationUrls).toEqual(["https://example.com/oauth"]);
    expect(messages).toEqual(["settings.library.dropboxSignInOpened"]);
  });
});
