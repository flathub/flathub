///@ts-check
import { readFile, writeFile, rm } from 'fs/promises';
import { join } from 'path';

/** @type {Record<string,string[]>} */
const stripInfo = JSON.parse(await readFile("generated/used_versions_strip_info.json"))

let flatpakManifestIndices = []

for (const packageName in stripInfo) {
    if (packageName === "pnpm") {
        continue
    }
    if (Object.prototype.hasOwnProperty.call(stripInfo, packageName)) {
        const usedVersions = stripInfo[packageName];
        const pathToIndex = join("generated/proxy-registry-cache-indices2", packageName, 'index.json')
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

        flatpakManifestIndices.push({
            "type": "inline",
            "contents": JSON.stringify(index),
            "dest": `npm-registry-proxy-offline-cache/${packageName}`,
            "dest-filename": "index.json"
        })
        
        // await rm(pathToIndex)
    }
}

// remove pnpm index - since it is propbabaly only needed for version check and we can live without that
try {
    const pathToManifest = "generated/proxy-registry-cache-manifest.json"
    let manifest = JSON.parse(await readFile(pathToManifest))
  
    // and sort
    manifest = manifest.sort((a, b) => {
        const pathA = a.path.toUpperCase();
        const pathB = b.path.toUpperCase();
        if (pathA < pathB) {
            return -1;
        }
        if (pathA > pathB) {
            return 1;
        }
        return 0;
    })

    await writeFile(pathToManifest, JSON.stringify(manifest, null, 2), 'utf-8')
} catch (error) { }

flatpakManifestIndices = flatpakManifestIndices.sort((a, b) => {
    const pathA = a.dest.toUpperCase();
    const pathB = b.dest.toUpperCase();
    if (pathA < pathB) {
        return -1;
    }
    if (pathA > pathB) {
        return 1;
    }
    return 0;
})
await writeFile("generated/proxy-registry-cache-indices.json", JSON.stringify(flatpakManifestIndices, null, 2), 'utf-8')