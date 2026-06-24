const PackageManager = {
  Yarn1: `Yarn Classic`,
  Yarn2: `Yarn`,
  Npm: `npm`,
  Pnpm: `pnpm`,
}

module.exports = {
  name: `flatpak-builder`,
  factory: require => {
    const { BaseCommand } = require(`@yarnpkg/cli`);
    const { parseSyml } = require('@yarnpkg/parsers');
    const { Configuration, Manifest, scriptUtils, structUtils, tgzUtils, execUtils, miscUtils, hashUtils } = require('@yarnpkg/core')
    const { Filename, ZipFS, npath, ppath, PortablePath, xfs } = require('@yarnpkg/fslib');
    const { getLibzipPromise } = require('@yarnpkg/libzip');
    const { gitUtils } = require('@yarnpkg/plugin-git');
    const { PassThrough, Readable, Writable } = require('stream');
    const { Command, Option } = require(`clipanion`);
    const { YarnVersion } = require('@yarnpkg/core');
    const fs = require('fs');

    // from https://github.com/yarnpkg/berry/blob/%40yarnpkg/shell/3.2.3/packages/plugin-essentials/sources/commands/set/version.ts#L194 
    async function setPackageManager(projectCwd) {
      const bundleVersion = YarnVersion;

      const manifest = (await Manifest.tryFind(projectCwd)) || new Manifest();

      if (bundleVersion && miscUtils.isTaggedYarnVersion(bundleVersion)) {
        manifest.packageManager = `yarn@${bundleVersion}`;
        const data = {};
        manifest.exportTo(data);

        const path = ppath.join(projectCwd, Manifest.fileName);
        const content = `${JSON.stringify(data, null, manifest.indent)}\n`;

        await xfs.changeFilePromise(path, content, {
          automaticNewlines: true,
        });
      }
    }

    // func from https://github.com/yarnpkg/berry/blob/%40yarnpkg/shell/3.2.3/packages/yarnpkg-core/sources/scriptUtils.ts#L215
    async function prepareExternalProject(cwd, outputPath, { configuration, locator, stdout, yarn_v1, workspace = null }) {
      const devirtualizedLocator = locator && structUtils.isVirtualLocator(locator)
        ? structUtils.devirtualizeLocator(locator)
        : locator;

      const name = devirtualizedLocator
        ? structUtils.stringifyLocator(devirtualizedLocator)
        : `an external project`;

      const stderr = stdout;

      stdout.write(`Packing ${name} from sources\n`);

      const packageManagerSelection = await scriptUtils.detectPackageManager(cwd);
      let effectivePackageManager;
      if (packageManagerSelection !== null) {
        stdout.write(`Using ${packageManagerSelection.packageManager} for bootstrap. Reason: ${packageManagerSelection.reason}\n\n`);
        effectivePackageManager = packageManagerSelection.packageManager;
      } else {
        stdout.write(`No package manager configuration detected; defaulting to Yarn\n\n`);
        effectivePackageManager = PackageManager.Yarn2;
      }
      if (effectivePackageManager === PackageManager.Pnpm) {
        effectivePackageManager = PackageManager.Npm;
      }

      const workflows = new Map([
        [PackageManager.Yarn1, async () => {
          const workspaceCli = workspace !== null
            ? [`workspace`, workspace]
            : [];

          await setPackageManager(cwd);

          await Configuration.updateConfiguration(cwd, {
            yarnPath: yarn_v1,
          });

          await xfs.appendFilePromise(ppath.join(cwd, `.npmignore`), `/.yarn\n`);

          const pack = await execUtils.pipevp(`yarn`, [...workspaceCli, `pack`, `--filename`, npath.fromPortablePath(outputPath)], { cwd, stdout, stderr });
          if (pack.code !== 0)
            return pack.code;

          return 0;
        }],
        [PackageManager.Yarn2, async () => {
          const workspaceCli = workspace !== null
            ? [`workspace`, workspace]
            : [];
          const lockfilePath = ppath.join(cwd, Filename.lockfile);
          if (!(await xfs.existsPromise(lockfilePath)))
            await xfs.writeFilePromise(lockfilePath, ``);

          const pack = await execUtils.pipevp(`yarn`, [...workspaceCli, `pack`, `--filename`, npath.fromPortablePath(outputPath)], { cwd, stdout, stderr });
          if (pack.code !== 0)
            return pack.code;
          return 0;
        }],
        [PackageManager.Npm, async () => {
          const workspaceCli = workspace !== null
            ? [`--workspace`, workspace]
            : [];
          const packStream = new PassThrough();
          const packPromise = miscUtils.bufferStream(packStream);
          const pack = await execUtils.pipevp(`npm`, [`pack`, `--silent`, ...workspaceCli], { cwd, stdout: packStream, stderr });
          if (pack.code !== 0)
            return pack.code;

          const packOutput = (await packPromise).toString().trim().replace(/^.*\n/s, ``);
          const packTarget = ppath.resolve(cwd, npath.toPortablePath(packOutput));
          await xfs.renamePromise(packTarget, outputPath);
          return 0;
        }],
      ]);
      const workflow = workflows.get(effectivePackageManager);
      const code = await workflow();
      if (code === 0 || typeof code === `undefined`)
        return;
      else
        throw `Packing the package failed (exit code ${code})`;
    }

    class convertToZipCommand extends BaseCommand {
      static paths = [[`convertToZip`]];
      yarn_v1 = Option.String({ required: true });

      async execute() {
        const configuration = await Configuration.find(this.context.cwd,
          this.context.plugins);
        const lockfilePath = ppath.join(this.context.cwd, 'yarn.lock');
        const cacheFolder = `${configuration.get('globalFolder')}/cache`;
        const locatorFolder = `${cacheFolder}/locator`;

        const compressionLevel = configuration.get(`compressionLevel`);
        const stdout = this.context.stdout;
        const gitChecksumPatches = []; // {name:, oriHash:, newHash:}

        async function patchLockfileChecksum(lockfilePath, patches) {
          let currentContent = ``;
          try {
            currentContent = await xfs.readFilePromise(lockfilePath, `utf8`);
          } catch (error) {
          }
          const newContent = patches.reduce((acc, item, i) => {
            stdout.write(`patch '${item.name}' checksum:\n-${item.oriHash}\n+${item.newHash}\n\n\n`);
            const regex = new RegExp(item.oriHash, "g");
            return acc.replace(regex, item.newHash);
          }, currentContent);

          await xfs.writeFilePromise(lockfilePath, newContent);
        }

        async function getLockFileMeta(lockfilePath) {
          const content = await xfs.readFilePromise(lockfilePath, `utf8`);
          const parsed = parseSyml(content);
          return parsed.__metadata;
        }

        const lockMeta = await getLockFileMeta(lockfilePath);
        stdout.write(`yarn lock:          ${lockfilePath}\n`);
        stdout.write(`yarn lock version:  ${lockMeta.version}\n`);
        stdout.write(`yarn lock cacheKey: ${lockMeta.cacheKey}\n`);

        const convertToZip = async (tgz, target, opts) => {
          const tgzBuf = await xfs.readFilePromise(tgz);
          const fs = await tgzUtils.convertToZip(tgzBuf, opts);
          fs.discardAndClose();
          await xfs.copyFilePromise(fs.path, target);
          await xfs.unlinkPromise(fs.path);
        }

        stdout.write(`converting tgz to zip: ${cacheFolder}\n`);

        const files = fs.readdirSync(locatorFolder);
        const tasks = []
        for (const i in files) {
          const file = `${files[i]}`;
          let tgzFile = `${locatorFolder}/${file}`;
          const match = file.match(/([^-]+)-([^.]{1,10})[.](tgz|git)$/);
          if (!match) {
            stdout.write(`ignore ${file}\n`);
            continue;
          }
          let resolution, locator;
          const entry_type = match[3];
          const sha = match[2];
          let checksum;

          if (entry_type === 'tgz') {
            resolution = Buffer.from(match[1], 'base64').toString();
            locator = structUtils.parseLocator(resolution, true);
          }
          else if (entry_type === 'git') {
            const gitJson = JSON.parse(fs.readFileSync(tgzFile, 'utf8'));

            resolution = gitJson.resolution;
            locator = structUtils.parseLocator(resolution, true);
            checksum = gitJson.checksum;

            const repoPathRel = gitJson.repo_dir_rel;

            const cloneTarget = `${cacheFolder}/${repoPathRel}`;

            const repoUrlParts = gitUtils.splitRepoUrl(locator.reference);
            const packagePath = ppath.join(cloneTarget, `package.tgz`);

            await prepareExternalProject(cloneTarget, packagePath, {
              configuration: configuration,
              stdout,
              workspace: repoUrlParts.extra.workspace,
              locator,
              yarn_v1: this.yarn_v1,
            });

            tgzFile = packagePath;

          }
          const filename =
            `${structUtils.slugifyLocator(locator)}-${lockMeta.cacheKey}.zip`;
          const targetFile = `${cacheFolder}/${filename}`

          tasks.push(async () => {
            await convertToZip(tgzFile, targetFile, {
              compressionLevel: compressionLevel,
              prefixPath: `node_modules/${structUtils.stringifyIdent(locator)}`,
              stripComponents: 1,
            });

            if (entry_type === 'git') {
              const file_checksum = await hashUtils.checksumFile(targetFile);

              if (file_checksum !== checksum) {
                const newSha = file_checksum.slice(0, 10);
                const newTarget = `${cacheFolder}/${structUtils.slugifyLocator(locator)}-${lockMeta.cacheKey}.zip`;
                fs.renameSync(targetFile, newTarget);

                gitChecksumPatches.push({
                  name: locator.name,
                  oriHash: checksum,
                  newHash: file_checksum,
                });
              }
            }
          });
        }

        await Promise.all(tasks.map(t => t()));

        patchLockfileChecksum(lockfilePath, gitChecksumPatches);
        stdout.write(`converting finished\n`);
      }
    }
    return {
      commands: [
        convertToZipCommand
      ],
    };
  }
};
