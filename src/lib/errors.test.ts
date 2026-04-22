import { describe, expect, test } from "vitest";
import { getErrorMessage } from "./errors";

describe("getErrorMessage", () => {
  test("returns the message from structured command-like error objects", () => {
    expect(
      getErrorMessage({
        code: "internal",
        message: "Google Drive folder creation failed.",
        retryable: false,
      }),
    ).toBe("Google Drive folder creation failed.");
  });
});
