// @vitest-environment jsdom

import { beforeEach, describe, expect, test } from "vitest";
import { applyShellDocumentMarker } from "./shell-document";

describe("applyShellDocumentMarker", () => {
  beforeEach(() => {
    document.documentElement.removeAttribute("data-app-shell");
    document.body.innerHTML = '<div id="root"></div>';
    document.body.removeAttribute("data-app-shell");
  });

  test("always stamps full-app on html, body, and root", () => {
    applyShellDocumentMarker();

    expect(document.documentElement.dataset.appShell).toBe("full-app");
    expect(document.body.dataset.appShell).toBe("full-app");
    expect(document.getElementById("root")?.dataset.appShell).toBe("full-app");
  });
});
