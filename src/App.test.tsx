import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import App from "./App";

vi.mock("@/runtime/app-runtime", () => ({
  useAppStartupRuntime: vi.fn(),
  useMainWindowRuntimeWhen: vi.fn(),
  useSidebarWindowRuntimeWhen: vi.fn(),
}));

vi.mock("@/components/Layout/AppLayout", () => ({
  AppLayout: () => <div data-testid="full-app-layout" />,
}));

vi.mock("@/components/Layout/SidebarWebviewApp", () => ({
  SidebarWebviewApp: () => <div data-testid="sidebar-webview-app" />,
}));

vi.mock("@/components/Layout/MainWebviewApp", () => ({
  MainWebviewApp: () => <div data-testid="main-webview-app" />,
}));

vi.mock("@/components/Settings/LibrarySetup", () => ({
  LibrarySetup: () => <div data-testid="library-setup" />,
}));

describe("App shell modes", () => {
  test("renders the shared full app layout by default", () => {
    const markup = renderToStaticMarkup(
      <App initialLibraryReady shellMode="full-app" />,
    );

    expect(markup).toContain('data-testid="full-app-layout"');
  });

  test("renders the sidebar-only shell for sidebar webviews", () => {
    const markup = renderToStaticMarkup(
      <App initialLibraryReady shellMode="sidebar-webview" />,
    );

    expect(markup).toContain('data-testid="sidebar-webview-app"');
    expect(markup).not.toContain('data-testid="full-app-layout"');
  });

  test("renders the main-content shell for main content webviews", () => {
    const markup = renderToStaticMarkup(
      <App initialLibraryReady shellMode="main-content-webview" />,
    );

    expect(markup).toContain('data-testid="main-webview-app"');
    expect(markup).not.toContain('data-testid="full-app-layout"');
  });

  test("keeps first-run setup out of the sidebar-only webview", () => {
    const markup = renderToStaticMarkup(
      <App initialLibraryReady={false} shellMode="sidebar-webview" />,
    );

    expect(markup).toBe("");
  });

  test("renders child shell hosts while startup state is still loading", () => {
    const sidebarMarkup = renderToStaticMarkup(
      <App initialLibraryReady={null} shellMode="sidebar-webview" />,
    );
    const mainMarkup = renderToStaticMarkup(
      <App initialLibraryReady={null} shellMode="main-content-webview" />,
    );

    expect(sidebarMarkup).toContain('data-testid="sidebar-webview-app"');
    expect(mainMarkup).toContain('data-testid="main-webview-app"');
  });

  test("shows library setup from the shared full-app shell only", () => {
    const markup = renderToStaticMarkup(
      <App initialLibraryReady={false} shellMode="full-app" />,
    );

    expect(markup).toContain('data-testid="library-setup"');
    expect(markup).not.toContain('data-testid="sidebar-webview-app"');
  });
});
