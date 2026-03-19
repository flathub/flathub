import { describe, expect, test } from "vitest";
import en from "./en.json";
import zh from "./zh-CN.json";

describe("locale copy", () => {
  test("uses the approved hide separate-all copy", () => {
    expect(en.settings.hideBatchSeparate.hide).toBe("Hide “Separate All”");
    expect(en.settings.hideBatchSeparate.description).toBe(
      "Hide the sidebar button that separates all songs.",
    );
    expect(zh.settings.hideBatchSeparate.hide).toBe("隐藏“全部分离”按钮");
    expect(zh.settings.hideBatchSeparate.description).toBe(
      "隐藏侧栏中用于分离全部歌曲的按钮。",
    );
  });

  test("uses an action label for the multi-select instrumental menu item", () => {
    expect(en.library.markInstrumentalSelected).toBe(
      "Mark as Instrumental ({{count}})",
    );
    expect(zh.library.markInstrumentalSelected).toBe("标记为伴奏 ({{count}})");
  });
});
