/**
 * Deletes the Chromium UI locales Trilium does not expose (~38 MB), mirroring
 * the electron-forge postPackage hook; runs after the Electron zip is unpacked
 * into /app. The keep-list comes from the checkout's own LOCALES, so a locale
 * added upstream flows through without a manifest change.
 */

import { readdirSync, unlinkSync } from "node:fs";

// Resolved relative to this script, which is staged next to the checkout.
import { LOCALES } from "./packages/commons/src/lib/i18n.ts";

const LOCALES_DIR = "/app/lib/electron/locales";

const keep = new Set(
    LOCALES.filter((l) => !l.contentOnly && l.electronLocale)
        .map((l) => `${(l.electronLocale ?? "").replace("_", "-")}.pak`)
);
keep.add("en-US.pak"); // Electron names English en-US.pak; the list has "en".

const dropped = readdirSync(LOCALES_DIR).filter((f) => !keep.has(f));
for (const f of dropped) {
    unlinkSync(`${LOCALES_DIR}/${f}`);
}
console.log(`Removed ${dropped.length} locales.`);
