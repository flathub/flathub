import { beforeEach, describe, expect, test } from "vitest";
import {
  createLayoutStore,
  useLayoutStore,
  type LayoutSyncSnapshot,
} from "./layout-store";
import { createWebviewSyncChannel } from "@/runtime/webview-sync";

interface FakeChannel {
  onmessage: ((event: { data: unknown }) => void) | null;
  postMessage: (data: unknown) => void;
  close: () => void;
}

describe("layout-store", () => {
  beforeEach(() => {
    useLayoutStore.setState({ sidebarVisible: true });
  });

  test("toggles sidebar visibility locally", () => {
    useLayoutStore.getState().toggleSidebar();

    expect(useLayoutStore.getState().sidebarVisible).toBe(false);
  });

  test("syncs sidebar visibility across webview contexts", () => {
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

    const primary = createLayoutStore(
      createWebviewSyncChannel<LayoutSyncSnapshot>("layout", {
        channelFactory,
        originId: "primary",
      }),
    );
    const secondary = createLayoutStore(
      createWebviewSyncChannel<LayoutSyncSnapshot>("layout", {
        channelFactory,
        originId: "secondary",
      }),
    );

    primary.store.getState().setSidebarVisible(false);

    expect(secondary.store.getState().sidebarVisible).toBe(false);

    primary.dispose();
    secondary.dispose();
  });
});
