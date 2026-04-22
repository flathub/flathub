import { describe, expect, test } from "vitest";
import { librarySetupChoices, remoteLibraryProviders } from "./LibrarySetup";

describe("LibrarySetup", () => {
  test("includes a direct remote-library entry in the first-run choices", () => {
    expect(librarySetupChoices.map((choice) => choice.kind)).toContain(
      "open_remote",
    );
  });

  test("exposes provider-specific remote-library entry points", () => {
    expect(remoteLibraryProviders.map((choice) => choice.provider)).toEqual([
      "google_drive",
      "dropbox",
      "webdav",
    ]);
  });

  test("marks Google Drive, Dropbox, and WebDAV as currently available provider paths", () => {
    const availableNow = remoteLibraryProviders
      .filter((choice) => choice.availableNow)
      .map((choice) => choice.provider);

    expect(availableNow).toEqual(["google_drive", "dropbox", "webdav"]);
  });
});
