import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import tailwindcss from "@tailwindcss/vite";
import { fileURLToPath, URL } from "node:url";
import { copyFileSync, mkdirSync } from "node:fs";
import { resolve, dirname } from "node:path";
import { createRequire } from "node:module";

// Tauri injects this value when remote device debugging is enabled.
// @ts-expect-error process is a nodejs global
const host = process.env.TAURI_DEV_HOST;

// Locate the kuromoji dictionary shipped with the lyric-romanizer dependency
// so it can be served as a static asset instead of fetched from jsDelivr CDN.
function kuromojiDictPlugin() {
  const DICT_FILES = [
    "base.dat.gz",
    "cc.dat.gz",
    "check.dat.gz",
    "tid.dat.gz",
    "tid_map.dat.gz",
    "tid_pos.dat.gz",
    "unk.dat.gz",
    "unk_char.dat.gz",
    "unk_compat.dat.gz",
    "unk_invoke.dat.gz",
    "unk_map.dat.gz",
    "unk_pos.dat.gz",
  ];

  return {
    name: "kuromoji-dict",
    configResolved() {
      const require = createRequire(import.meta.url);
      const kuromojiMain = require.resolve("@sglkc/kuromoji");
      const kuromojiRoot = resolve(dirname(kuromojiMain), "..");
      const srcDir = resolve(kuromojiRoot, "dict");
      const destDir = resolve(
        fileURLToPath(new URL(".", import.meta.url)),
        "public",
        "dict",
      );
      mkdirSync(destDir, { recursive: true });
      for (const file of DICT_FILES) {
        copyFileSync(resolve(srcDir, file), resolve(destDir, file));
      }
    },
  };
}

// https://vite.dev/config/
export default defineConfig(async () => ({
  plugins: [react(), tailwindcss(), kuromojiDictPlugin()],
  test: {
    // Nested git worktrees under `.worktrees/` duplicate `src/**` and must not
    // be collected as part of this package's unit test run.
    include: [
      "src/**/*.{test,spec}.?(c|m)[jt]s?(x)",
      "tests/**/*.{test,spec}.?(c|m)[jt]s?(x)",
    ],
    exclude: ["**/node_modules/**", "**/dist/**", "**/.worktrees/**"],
    setupFiles: ["./src/test-setup.ts"],
  },
  resolve: {
    alias: {
      "@": fileURLToPath(new URL("./src", import.meta.url)),
    },
  },
  build: {
    chunkSizeWarningLimit: 1000,
  },

  // Vite options tailored for Tauri development and only applied in `tauri dev` or `tauri build`
  //
  // 1. prevent Vite from obscuring rust errors
  clearScreen: false,
  // 2. tauri expects a fixed port, fail if that port is not available
  server: {
    port: 1420,
    strictPort: true,
    host: host || false,
    hmr: host
      ? {
          protocol: "ws",
          host,
          port: 1421,
        }
      : undefined,
    watch: {
      // 3. tell Vite to ignore watching `src-tauri`
      ignored: ["**/src-tauri/**"],
    },
  },
}));
