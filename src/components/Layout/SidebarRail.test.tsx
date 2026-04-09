import { renderToStaticMarkup } from "react-dom/server";
import { describe, expect, test } from "vitest";
import { SidebarRail } from "./SidebarRail";

describe("SidebarRail", () => {
  test("keeps the rail mounted with a zero-width shell when hidden", () => {
    const markup = renderToStaticMarkup(
      <SidebarRail visible={false} width={300} onResize={() => {}}>
        <div>Sidebar</div>
      </SidebarRail>,
    );

    expect(markup).toContain("overflow-hidden");
    expect(markup).toContain("transition-[width]");
    expect(markup).toContain("w-0");
    expect(markup).toContain("opacity-0");
    expect(markup).toContain("-translate-x-3");
  });

  test("expands the rail and restores the content transform when visible", () => {
    const markup = renderToStaticMarkup(
      <SidebarRail visible width={300} onResize={() => {}}>
        <div>Sidebar</div>
      </SidebarRail>,
    );

    expect(markup).toContain("w-[var(--window-shell-sidebar-width)]");
    expect(markup).toContain("opacity-100");
    expect(markup).toContain("translate-x-0");
    expect(markup).toContain('role="separator"');
  });
});
