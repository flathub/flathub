import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test } from "vitest";
import { SettingsLibrarySection } from "./SettingsLibrarySection";
import {
  SettingsOverlayContext,
  createSettingsOverlayTestContextValue,
} from "./SettingsOverlay.context";

describe("SettingsLibrarySection", () => {
  test("renders provider metadata for remote libraries", () => {
    const value = createSettingsOverlayTestContextValue({
      state: {
        libraries: [
          {
            id: "local:/karaoke",
            kind: "local",
            display_name: "Main Library",
            root_path: "/karaoke",
          },
          {
            id: "remote:drive",
            kind: "remote",
            display_name: "Drive Library",
            provider: "webdav",
            remote_root_locator: "drive-root",
            remote_path_display: "OpenKara / Team Karaoke",
            account_id: "acct-1",
            connection_config: {
              type: "webdav",
              server_url: "https://dav.example.com/remote.php/dav/files/user/",
            },
            cached_db_path: null,
            remote_revision: null,
          },
        ],
        activeLibraryId: "remote:drive",
      },
    });

    const markup = renderToStaticMarkup(
      <SettingsOverlayContext value={value}>
        <SettingsLibrarySection />
      </SettingsOverlayContext>,
    );

    expect(markup).toContain("Drive Library");
    expect(markup).toContain("WebDAV");
    expect(markup).toContain("OpenKara / Team Karaoke");
    expect(markup).toContain("Add Remote Repository");
  });

  test("renders library management actions separately from switching", () => {
    const value = createSettingsOverlayTestContextValue({
      state: {
        libraries: [
          {
            id: "local:/karaoke",
            kind: "local",
            display_name: "Main Library",
            root_path: "/karaoke",
          },
          {
            id: "remote:drive",
            kind: "remote",
            display_name: "Drive Library",
            provider: "dropbox",
            remote_root_locator: "/OpenKara",
            remote_path_display: "/OpenKara",
            account_id: "acct-1",
            connection_config: null,
            cached_db_path: "/tmp/openkara.db",
            remote_revision: null,
          },
        ],
        activeLibraryId: "local:/karaoke",
      },
    });

    const markup = renderToStaticMarkup(
      <SettingsOverlayContext value={value}>
        <SettingsLibrarySection />
      </SettingsOverlayContext>,
    );

    expect(markup).toContain("Rename library");
    expect(markup).toContain("Disconnect repository");
    expect(markup).toContain("Delete repository");
    expect(markup).toContain("Refresh remote repository");
    expect(markup).toContain("Reauthorize remote repository");
    expect(markup).not.toContain("Force resync remote library");
    expect(markup).not.toContain("Reconnect provider");
    expect(markup).not.toContain("Update credentials");
  });
});
