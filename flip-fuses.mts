/**
 * Flips the same Electron fuses the electron-forge builds ship, minus the two
 * asar ones (`OnlyLoadAppFromAsar`, `EnableEmbeddedAsarIntegrityValidation`):
 * this build loads the app from a plain directory, which those would forbid.
 * Runs after the Electron zip is unpacked and the binary renamed.
 */

// Resolved from the checkout's node_modules; @electron/fuses is a dependency
// of electron-forge, hoisted to the root.
import { flipFuses, FuseV1Options, FuseVersion } from "@electron/fuses";

const BINARY_PATH = "/app/lib/electron/trilium";

await flipFuses(BINARY_PATH, {
    version: FuseVersion.V1,
    [FuseV1Options.RunAsNode]: false,
    [FuseV1Options.EnableNodeOptionsEnvironmentVariable]: false,
    [FuseV1Options.EnableNodeCliInspectArguments]: false,
    [FuseV1Options.EnableCookieEncryption]: true,
    [FuseV1Options.GrantFileProtocolExtraPrivileges]: false
});
console.log(`Flipped fuses on ${BINARY_PATH}`);
