import * as fs from 'node:fs';
import * as path from 'node:path';

const outputFile = path.join(__dirname, '..', 'static', 'releases.json');

export async function populateReleases() {
  const sourceDir = process.env.FLATPAK_BUILDER_BUILDDIR;

  if (!sourceDir) {
    throw new Error('FLATPAK_BUILDER_BUILDDIR is not set.');
  }

  const inputFile = path.resolve(sourceDir, 'releases.json');
  const data = await fs.promises.readFile(inputFile, 'utf-8');
  await fs.promises.writeFile(outputFile, data);
}

if (require.main === module) {
  (async () => {
    await populateReleases();
  })();
}
