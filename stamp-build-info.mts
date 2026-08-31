/**
 * Writes the built commit's date and hash into the module behind the About
 * dialog; runs before `pnpm desktop:build`. The repo's own update-build-info
 * script stamps the wall-clock time instead, which would differ on every
 * rebuild of the same source.
 */

import { execSync } from "node:child_process";
import { writeFileSync } from "node:fs";

const revision = git("rev-parse HEAD");
const commitEpoch = Number(git("log -1 --format=%ct"));
const buildDate = new Date(commitEpoch * 1000).toISOString().replace(".000", "");

const buildInfo = `\
export default {
    buildDate: "${buildDate}",
    buildRevision: "${revision}"
};
`;

writeFileSync("packages/trilium-core/src/services/build.ts", buildInfo);
console.log(`Stamped ${buildDate} @ ${revision}`);

function git(args: string): string {
    return execSync(`git ${args}`).toString().trim();
}
