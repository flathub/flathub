import { beforeEach, describe, expect, test } from "vitest";
import { createWebviewSyncChannel } from "@/runtime/webview-sync";
import {
  createSettingsStore,
  type SettingsSyncSnapshot,
  useSettingsStore,
} from "./settings-store";

interface FakeChannel {
  onmessage: ((event: { data: unknown }) => void) | null;
  postMessage: (data: unknown) => void;
  close: () => void;
}

describe("settings-store sync", () => {
  beforeEach(() => {
    useSettingsStore.setState({ isOpen: false });
  });

  test("syncs settings overlay visibility across webview contexts", () => {
    const channelsByName = new Map<string, Set<FakeChannel>>();
    const channelFactory = (name: string) => {
      const peers = channelsByName.get(name) ?? new Set<FakeChannel>();
      channelsByName.set(name, peers);

      const channel: FakeChannel = {
        onmessage: null,
        postMessage(data: unknown) {
          for (const peer of peers) {
            if (peer === channel) {
              continue;
            }
            peer.onmessage?.({ data });
          }
        },
        close() {
          peers.delete(channel);
        },
      };

      peers.add(channel);
      return channel;
    };

    const primary = createSettingsStore(
      createWebviewSyncChannel<SettingsSyncSnapshot>("settings", {
        channelFactory,
        originId: "primary",
      }),
    );
    const secondary = createSettingsStore(
      createWebviewSyncChannel<SettingsSyncSnapshot>("settings", {
        channelFactory,
        originId: "secondary",
      }),
    );

    primary.store.getState().open();

    expect(secondary.store.getState().isOpen).toBe(true);

    primary.dispose();
    secondary.dispose();
  });

  // NOTE: macOS shell selection was removed; settings sync now only covers shared prefs.
});
