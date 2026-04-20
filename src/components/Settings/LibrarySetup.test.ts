import { describe, expect, test } from "vitest";
import { librarySetupChoices } from "./LibrarySetup";

describe("LibrarySetup", () => {
  test("includes a direct remote-library entry in the first-run choices", () => {
    expect(librarySetupChoices.map((choice) => choice.kind)).toContain(
      "open_remote",
    );
  });
});
