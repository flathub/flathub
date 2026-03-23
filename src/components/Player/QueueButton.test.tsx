import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { QueueButton } from "./QueueButton";

const { mockQueueState } = vi.hoisted(() => ({
  mockQueueState: {
    queue: [],
    togglePanel: vi.fn(),
    isOpen: false,
  },
}));

vi.mock("react-i18next", () => ({
  useTranslation: () => ({
    t: (key: string) => key,
  }),
}));

vi.mock("@/stores/queue-store", () => ({
  useQueueStore: (selector: (state: typeof mockQueueState) => unknown) =>
    selector(mockQueueState),
}));

vi.mock("@/components/Overlay/Tooltip", () => ({
  Tooltip: ({ children }: { children: React.ReactNode }) => children,
}));

describe("QueueButton", () => {
  test("exposes the native utility variant in mac native playback chrome", () => {
    const markup = renderToStaticMarkup(<QueueButton shellTier="mac_native" />);

    expect(markup).toContain('data-queue-button-visual-variant="native"');
    expect(markup).toContain('aria-label="queue.title"');
  });
});
