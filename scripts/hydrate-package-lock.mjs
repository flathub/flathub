#!/usr/bin/env node

import { readFile, writeFile } from 'node:fs/promises';

const [inputPath, outputPath = inputPath] = process.argv.slice(2);
if (!inputPath) {
    console.error('usage: hydrate-package-lock.mjs INPUT [OUTPUT]');
    process.exit(2);
}

const lock = JSON.parse(await readFile(inputPath, 'utf8'));
const missing = new Map();

for (const [path, metadata] of Object.entries(lock.packages ?? {})) {
    if (!path || metadata.link || !metadata.version || (metadata.resolved && metadata.integrity)) {
        continue;
    }

    // npm aliases keep their real registry name in the entry's `name` field.
    const name = metadata.name ?? path.split('node_modules/').at(-1);
    const key = `${name}@${metadata.version}`;
    const entries = missing.get(key) ?? [];
    entries.push(metadata);
    missing.set(key, entries);
}

const jobs = [...missing.entries()];
let nextJob = 0;

async function hydrate() {
    while (nextJob < jobs.length) {
        const [key, entries] = jobs[nextJob++];
        const sample = entries[0];
        const name = key.slice(0, -(sample.version.length + 1));
        const url = `https://registry.npmjs.org/${encodeURIComponent(name)}/${encodeURIComponent(sample.version)}`;
        const response = await fetch(url);
        if (!response.ok) {
            throw new Error(`${response.status} ${response.statusText}: ${url}`);
        }

        const registry = await response.json();
        if (!registry.dist?.tarball || !registry.dist?.integrity) {
            throw new Error(`missing dist metadata: ${key}`);
        }

        for (const metadata of entries) {
            metadata.resolved ??= registry.dist.tarball;
            metadata.integrity ??= registry.dist.integrity;
        }
    }
}

await Promise.all(Array.from({ length: Math.min(16, jobs.length) }, hydrate));
await writeFile(outputPath, `${JSON.stringify(lock, null, 4)}\n`);
console.log(`Hydrated ${jobs.length} package version(s).`);
