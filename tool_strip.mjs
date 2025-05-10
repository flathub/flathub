///@ts-check
import { readFile, writeFile, rm } from 'fs/promises';
import { join } from 'path';

/** @type {Record<string,string[]>} */
const stripInfo = JSON.parse(await readFile("generated/used_versions_strip_info.json"))

for (const packageName in stripInfo) {
    if (packageName === "pnpm") {
        continue
    }
    if (Object.prototype.hasOwnProperty.call(stripInfo, packageName)) {
        const usedVersions = stripInfo[packageName];
        const pathToIndex = join("generated/proxy-registry-cache-indices", packageName, 'index.json')
        const index = JSON.parse(await readFile(pathToIndex))
        const allVersions = Object.keys(index["versions"])

        const unusedVersions = allVersions.filter(version => !usedVersions.includes(version))
        // console.log({allVersions, unusedVersions});

        unusedVersions.forEach(version => {
            delete index["versions"][version]
            delete index["time"][version]
        })

        // Theoretical issue: we may need to adjust "time" object's modified property to the latest available version after filtering

        delete index["users"]

        await writeFile(pathToIndex, JSON.stringify(index, null, 2), 'utf-8')
    }
}

// remove pnpm index - since it is propbabaly only needed for version check and we can live without that
try {
    await rm(join("generated/proxy-registry-cache-indices", 'pnpm', 'index.json'))
    const pathToManifest = "generated/generated/proxy-registry-cache-manifest.json"
    let manifest = JSON.parse(await readFile(pathToManifest))
    manifest = manifest.filter(entry => entry.path != "generated/proxy-registry-cache-indices/pnpm/index.json")
    await writeFile(pathToManifest, JSON.stringify(manifest, null, 2), 'utf-8')
} catch (error) { }