import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test, vi } from "vitest";
import { AudioLevelSlider } from "./AudioLevelSlider";

vi.mock("@/components/Overlay/Tooltip", () => ({
  Tooltip: ({
    children,
    label,
  }: {
    children: React.ReactNode;
    label: string;
  }) => <span data-tooltip-label={label}>{children}</span>,
}));

describe("AudioLevelSlider", () => {
  test("renders an immediate tooltip label with the current percentage", () => {
    const markup = renderToStaticMarkup(
      <AudioLevelSlider
        label="Volume"
        value={0.72}
        onChange={() => {}}
        widthClass="w-20"
      />,
    );

    expect(markup).toContain('data-tooltip-label="Volume 72%"');
    expect(markup).toContain('class="native-slider audio-level-slider w-20"');
    expect(markup).not.toContain("title=");
  });

  test("preserves disabled state and aria labelling without native title", () => {
    const markup = renderToStaticMarkup(
      <AudioLevelSlider
        label="Drums"
        value={0.35}
        onChange={() => {}}
        disabled
        ariaLabel="Drums"
      />,
    );

    expect(markup).toContain('data-tooltip-label="Drums 35%"');
    expect(markup).toContain("disabled");
    expect(markup).toContain('aria-label="Drums"');
    expect(markup).not.toContain("title=");
  });
});
