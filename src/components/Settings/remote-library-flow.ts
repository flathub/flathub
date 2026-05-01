import type { TFunction } from "i18next";
import * as api from "@/lib/tauri";
import type {
  LibraryRegistrySnapshot,
  RemoteAuthPayload,
  RemoteAuthStart,
  RemoteLibraryCandidate,
  RemoteLibraryProvider,
} from "@/types/ipc";
import {
  getRemoteProviderAuthTimeoutMessage,
  getRemoteProviderBrowserSignInOpenedMessage,
  getRemoteProviderDisplayName,
} from "./remote-library-copy";

export const REMOTE_AUTH_TIMEOUT_MS = 120_000;
export const REMOTE_AUTH_POLL_INTERVAL_MS = 1_000;
export const REMOTE_AUTH_CANCELLED = "__remote_auth_cancelled__";

export interface WebDavRemoteLibraryFields {
  serverUrl: string;
  username: string;
  password: string;
  rootPath: string;
}

export interface RemoteLibraryFlowApi {
  beginRemoteAuth: typeof api.beginRemoteAuth;
  openExternalUrl: typeof api.openExternalUrl;
  pollRemoteAuth: typeof api.pollRemoteAuth;
  createRemoteLibrary: typeof api.createRemoteLibrary;
  resolveRemoteLibraryCandidate: typeof api.resolveRemoteLibraryCandidate;
  registerRemoteLibrary: typeof api.registerRemoteLibrary;
  reauthorizeRemoteLibrary: typeof api.reauthorizeRemoteLibrary;
  cancelRemoteAuth: typeof api.cancelRemoteAuth;
}

interface PollRemoteAuthUntilReadyOptions {
  sessionId: string;
  provider: RemoteLibraryProvider;
  t: TFunction;
  remoteApi?: Pick<RemoteLibraryFlowApi, "pollRemoteAuth">;
  isCancelled?: () => boolean;
  now?: () => number;
  wait?: (durationMs: number) => Promise<void>;
}

export interface RunRemoteLibraryRegistrationFlowOptions {
  provider: RemoteLibraryProvider;
  displayName: string;
  t: TFunction;
  libraryId?: string;
  existingRemoteRootLocator?: string;
  existingRemotePathDisplay?: string;
  allowRelocation?: boolean;
  webdav?: WebDavRemoteLibraryFields;
  remoteApi?: RemoteLibraryFlowApi;
  isCancelled?: () => boolean;
  onSessionIdChange?: (sessionId: string | null) => void;
  onAuthorizationUrlChange?: (authorizationUrl: string | null) => void;
  onMessageChange?: (message: string | null) => void;
}

export interface RemoteLibraryRegistrationFlowResult {
  start: RemoteAuthStart;
  candidate: RemoteLibraryCandidate;
  registry: LibraryRegistrySnapshot;
}

const defaultRemoteLibraryFlowApi: RemoteLibraryFlowApi = {
  beginRemoteAuth: api.beginRemoteAuth,
  openExternalUrl: api.openExternalUrl,
  pollRemoteAuth: api.pollRemoteAuth,
  createRemoteLibrary: api.createRemoteLibrary,
  resolveRemoteLibraryCandidate: api.resolveRemoteLibraryCandidate,
  registerRemoteLibrary: api.registerRemoteLibrary,
  reauthorizeRemoteLibrary: api.reauthorizeRemoteLibrary,
  cancelRemoteAuth: api.cancelRemoteAuth,
};

function defaultWait(durationMs: number) {
  return new Promise<void>((resolve) => {
    window.setTimeout(resolve, durationMs);
  });
}

function ensureRemoteAuthNotCancelled(isCancelled?: () => boolean) {
  if (isCancelled?.()) {
    throw new Error(REMOTE_AUTH_CANCELLED);
  }
}

export function validateWebDavRemoteLibraryFields(
  t: TFunction,
  fields: Pick<
    WebDavRemoteLibraryFields,
    "serverUrl" | "username" | "password"
  >,
): string | null {
  if (!fields.serverUrl.trim()) {
    return t("settings.library.webdavEnterServerUrl", {
      defaultValue: "Enter the WebDAV server URL first.",
    });
  }

  if (!fields.username.trim()) {
    return t("settings.library.webdavEnterUsername", {
      defaultValue: "Enter the WebDAV username first.",
    });
  }

  if (!fields.password.trim()) {
    return t("settings.library.webdavEnterPassword", {
      defaultValue: "Enter the WebDAV password first.",
    });
  }

  return null;
}

export async function pollRemoteAuthUntilReady({
  sessionId,
  provider,
  t,
  remoteApi = defaultRemoteLibraryFlowApi,
  isCancelled,
  now = Date.now,
  wait = defaultWait,
}: PollRemoteAuthUntilReadyOptions): Promise<void> {
  const deadline = now() + REMOTE_AUTH_TIMEOUT_MS;

  while (now() < deadline) {
    ensureRemoteAuthNotCancelled(isCancelled);
    await wait(REMOTE_AUTH_POLL_INTERVAL_MS);
    ensureRemoteAuthNotCancelled(isCancelled);

    const status = await remoteApi.pollRemoteAuth(sessionId);
    if (status.state === "ready") {
      return;
    }

    if (status.state === "failed") {
      throw new Error(
        status.error?.message ??
          t("settings.library.remoteSignInFailedUnexpectedly", {
            defaultValue: "Remote sign-in failed unexpectedly.",
          }),
      );
    }
  }

  throw new Error(getRemoteProviderAuthTimeoutMessage(t, provider));
}

export async function runRemoteLibraryRegistrationFlow({
  provider,
  displayName,
  t,
  libraryId,
  existingRemoteRootLocator,
  existingRemotePathDisplay,
  allowRelocation = false,
  webdav,
  remoteApi = defaultRemoteLibraryFlowApi,
  isCancelled,
  onSessionIdChange,
  onAuthorizationUrlChange,
  onMessageChange,
}: RunRemoteLibraryRegistrationFlowOptions): Promise<RemoteLibraryRegistrationFlowResult> {
  if (provider === "webdav") {
    const validationError = validateWebDavRemoteLibraryFields(
      t,
      webdav ?? {
        serverUrl: "",
        username: "",
        password: "",
      },
    );
    if (validationError) {
      throw new Error(validationError);
    }
  }

  let startedSessionId: string | null = null;

  try {
    const payload: RemoteAuthPayload =
      provider === "webdav" && webdav
        ? {
            type: "webdav",
            server_url: webdav.serverUrl,
            username: webdav.username,
            password: webdav.password,
            root_path: webdav.rootPath.trim() || null,
          }
        : null;
    const start = await remoteApi.beginRemoteAuth(provider, payload);
    startedSessionId = start.session_id;
    onSessionIdChange?.(start.session_id);

    if (start.authorization_url) {
      onAuthorizationUrlChange?.(start.authorization_url);
      onMessageChange?.(
        getRemoteProviderBrowserSignInOpenedMessage(t, provider),
      );
      await remoteApi.openExternalUrl(start.authorization_url);
      await pollRemoteAuthUntilReady({
        sessionId: start.session_id,
        provider,
        t,
        remoteApi,
        isCancelled,
      });
    }

    ensureRemoteAuthNotCancelled(isCancelled);

    const requestedDisplayName =
      displayName.trim() || getRemoteProviderDisplayName(t, provider);
    const candidate =
      libraryId && provider !== "webdav"
        ? {
            provider,
            remote_root_locator: existingRemoteRootLocator ?? "",
            remote_path_display:
              existingRemotePathDisplay ?? existingRemoteRootLocator ?? "",
            display_name: requestedDisplayName,
            account_id: "",
          }
        : libraryId
          ? await remoteApi.resolveRemoteLibraryCandidate(
              start.session_id,
              requestedDisplayName,
            )
          : await remoteApi.createRemoteLibrary(
              start.session_id,
              requestedDisplayName,
            );
    const nextDisplayName = displayName.trim() || candidate.display_name;
    const registry = libraryId
      ? await remoteApi.reauthorizeRemoteLibrary(
          libraryId,
          start.session_id,
          candidate.remote_root_locator,
          nextDisplayName,
          allowRelocation,
        )
      : await remoteApi.registerRemoteLibrary(
          start.session_id,
          candidate.remote_root_locator,
          nextDisplayName,
        );

    return {
      start,
      candidate,
      registry,
    };
  } catch (error) {
    if (startedSessionId) {
      void remoteApi.cancelRemoteAuth(startedSessionId);
    }
    throw error;
  } finally {
    onSessionIdChange?.(null);
  }
}
