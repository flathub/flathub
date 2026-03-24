import type { ReactElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { SettingsDangerZoneSection } from "./SettingsDangerZoneSection";
import { SettingsOverlay } from "./SettingsOverlay";
import { SettingsDialogHost } from "./SettingsDialogHost";
import { SettingsGeneralSection } from "./SettingsGeneralSection";
import { SettingsLibrarySection } from "./SettingsLibrarySection";
import { SettingsModelVariantSection } from "./SettingsModelVariantSection";
import {
  SettingsOverlayContext,
  createSettingsOverlayTestContextValue,
  type SettingsOverlayContextValue,
} from "./SettingsOverlay.context";
import { SettingsStemModeSection } from "./SettingsStemModeSection";

vi.mock("@/lib/app-shortcuts", () => ({
  getShortcutPlatform: () => "mac",
}));

const { mockSettingsStore } = vi.hoisted(() => ({
  mockSettingsStore: {
    close: vi.fn(),
  },
}));

vi.mock("./SettingsOverlay.controller", async () => {
  const actual = await import("./SettingsOverlay.context");

  return {
    SettingsOverlayProvider: ({ children }: { children: React.ReactNode }) => (
      <actual.SettingsOverlayContext
        value={actual.createSettingsOverlayTestContextValue()}
      >
        {children}
      </actual.SettingsOverlayContext>
    ),
  };
});

vi.mock("@/stores/settings-store", () => ({
  useSettingsStore: Object.assign(
    (selector: (state: typeof mockSettingsStore) => unknown) =>
      selector(mockSettingsStore),
    {
      getState: () => ({
        getAppSettingsSnapshot: () => ({
          stemMode: "two_stem",
          modelVariant: "htdemucs",
          language: "en",
          hideBatchSeparate: false,
          lyricsFontStep: 0,
          macosShellMode: "stable",
        }),
      }),
    },
  ),
}));

vi.mock("react-i18next", async (importOriginal) => {
  const actual = await importOriginal<typeof import("react-i18next")>();

  return {
    ...actual,
    useTranslation: () => ({
      t: (key: string, params?: Record<string, string>) =>
        params?.size ? `${key}:${params.size}` : key,
      i18n: { changeLanguage: vi.fn() },
    }),
  };
});

function renderWithSettingsContext(
  node: ReactElement,
  value: SettingsOverlayContextValue,
) {
  return renderToStaticMarkup(
    <SettingsOverlayContext value={value}>{node}</SettingsOverlayContext>,
  );
}

describe("SettingsOverlay sections", () => {
  test("renders all primary sections", () => {
    const value = createSettingsOverlayTestContextValue();

    const markup = renderWithSettingsContext(
      <>
        <SettingsLibrarySection />
        <SettingsStemModeSection />
        <SettingsModelVariantSection />
        <SettingsGeneralSection />
        <SettingsDangerZoneSection />
      </>,
      value,
    );

    expect(markup).toContain("settings.library.label");
    expect(markup).toContain("settings.stemMode.label");
    expect(markup).toContain("settings.modelVariant.label");
    expect(markup).toContain("settings.language.label");
    expect(markup).toContain("settings.macosShellMode.label");
    expect(markup).toContain("settings.dangerZone.label");
  });

  test("renders the macOS shell mode choices on macOS", () => {
    const value = createSettingsOverlayTestContextValue();

    const markup = renderWithSettingsContext(<SettingsGeneralSection />, value);

    expect(markup).toContain("settings.macosShellMode.stable");
    expect(markup).toContain("settings.macosShellMode.native");
  });

  test("renders the restart button when the desired shell mode differs from the running shell", () => {
    const value = createSettingsOverlayTestContextValue({
      state: {
        macosShellMode: "native",
      },
      meta: {
        isInitializing: false,
        appliedMacOsShellMode: "stable",
      },
    });

    const markup = renderWithSettingsContext(<SettingsGeneralSection />, value);

    expect(markup).toContain("settings.macosShellMode.restartButton");
  });

  test("renders downloaded, downloading, and not-downloaded model statuses", () => {
    const value = createSettingsOverlayTestContextValue({
      state: {
        modelStatuses: {
          htdemucs: { downloaded: true, file_size: 1024 },
          htdemucs_ft: { downloaded: false, file_size: null },
        },
        downloadingModel: "htdemucs_ft",
      },
    });

    const markup = renderWithSettingsContext(
      <SettingsModelVariantSection />,
      value,
    );

    expect(markup).toContain("settings.modelVariant.downloaded");
    expect(markup).toContain("1.0 KB");
    expect(markup).toContain("settings.modelVariant.downloading");
  });

  test("danger zone hides model deletion actions when models are not downloaded", () => {
    const value = createSettingsOverlayTestContextValue({
      state: {
        modelStatuses: {
          htdemucs: { downloaded: false, file_size: null },
          htdemucs_ft: { downloaded: false, file_size: null },
        },
      },
    });

    const markup = renderWithSettingsContext(
      <SettingsDangerZoneSection />,
      value,
    );

    expect(markup).not.toContain("settings.dangerZone.deleteModelStandard");
    expect(markup).not.toContain("settings.dangerZone.deleteModelHQ");
  });

  test("dialog host renders the active dialog", () => {
    const value = createSettingsOverlayTestContextValue({
      meta: {
        dangerDialog: "ft_warning",
      },
    });

    const markup = renderWithSettingsContext(<SettingsDialogHost />, value);

    expect(markup).toContain("settings.modelVariant.ftWarningTitle");
    expect(markup).toContain("settings.modelVariant.ftWarningConfirm");
  });

  test("settings overlay renders a close control for mouse users", () => {
    const markup = renderToStaticMarkup(<SettingsOverlay />);

    expect(markup).toContain('aria-label="common.close"');
  });
});
