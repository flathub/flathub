import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import App from "./App";

vi.mock("@/runtime/app-runtime", () => ({
  useAppStartupRuntime: vi.fn(),
  useAppRuntime: vi.fn(),
}));

vi.mock("@/components/Layout/AppLayout", () => ({
  AppLayout: () => <div data-testid="full-app-layout" />,
}));

vi.mock("@/components/Settings/LibrarySetup", () => ({
  LibrarySetup: () => <div data-testid="library-setup" />,
}));

describe("App", () => {
  test("renders the shared full app layout when the library is ready", () => {
    const markup = renderToStaticMarkup(<App initialLibraryReady />);

    expect(markup).toContain('data-testid="full-app-layout"');
    expect(markup).not.toContain('data-testid="library-setup"');
  });

  test("shows library setup when no library is configured", () => {
    const markup = renderToStaticMarkup(<App initialLibraryReady={false} />);

    expect(markup).toContain('data-testid="library-setup"');
    expect(markup).not.toContain('data-testid="full-app-layout"');
  });

  test("renders nothing while the library path probe is still pending", () => {
    const markup = renderToStaticMarkup(<App initialLibraryReady={null} />);

    expect(markup).toBe("");
  });
});
