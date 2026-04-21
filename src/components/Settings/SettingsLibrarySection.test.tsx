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
            bound_local_library_id: "local:/karaoke",
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
    expect(markup).toContain("Mirroring Main Library");
    expect(markup).toContain("Add Remote Library");
  });
});
