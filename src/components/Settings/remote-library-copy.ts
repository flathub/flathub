import type { TFunction } from "i18next";
import type { RemoteLibraryProvider } from "@/types/ipc";

export function getRemoteProviderDisplayName(
  t: TFunction,
  provider: RemoteLibraryProvider,
): string {
  void provider;
  return t("settings.library.remoteLibraryDisplayName", {
    defaultValue: "OpenKara",
  });
}

export function getRemoteProviderLabel(
  t: TFunction,
  provider: RemoteLibraryProvider,
): string {
  return provider === "google_drive"
    ? t("setup.remoteProvider.googleDrive.title", {
        defaultValue: "Google Drive",
      })
    : provider === "dropbox"
      ? t("setup.remoteProvider.dropbox.title", {
          defaultValue: "Dropbox",
        })
      : t("setup.remoteProvider.webdav.title", {
          defaultValue: "WebDAV",
        });
}

export function getRemoteProviderBrowserSignInOpenedMessage(
  t: TFunction,
  provider: RemoteLibraryProvider,
): string | null {
  if (provider === "google_drive") {
    return t("settings.library.googleSignInOpened", {
      defaultValue:
        "Google sign-in opened in your browser. Finish the consent flow and OpenKara will continue automatically.",
    });
  }

  if (provider === "dropbox") {
    return t("settings.library.dropboxSignInOpened", {
      defaultValue:
        "Dropbox sign-in opened in your browser. Finish the consent flow and OpenKara will continue automatically.",
    });
  }

  return null;
}

export function getRemoteProviderAuthTimeoutMessage(
  t: TFunction,
  provider: RemoteLibraryProvider,
): string {
  return provider === "google_drive"
    ? t("settings.library.googleSignInTimedOut", {
        defaultValue:
          "Google sign-in timed out before OpenKara received the callback.",
      })
    : provider === "dropbox"
      ? t("settings.library.dropboxSignInTimedOut", {
          defaultValue:
            "Dropbox sign-in timed out before OpenKara received the callback.",
        })
      : t("settings.library.remoteSignInTimedOut", {
          defaultValue: "Remote sign-in timed out.",
        });
}

export function getRemoteLibraryConnectedMessage(
  t: TFunction,
  provider: RemoteLibraryProvider,
): string {
  return t("settings.library.remoteLibraryConnected", {
    defaultValue:
      provider === "google_drive"
        ? "Google Drive library connected successfully."
        : provider === "dropbox"
          ? "Dropbox library connected successfully."
          : provider === "webdav"
            ? "WebDAV library connected successfully."
            : "Remote library connected successfully.",
  });
}
