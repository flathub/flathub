// Generate the Flatpak launcher from the desktop metadata in upstream's
// electron-builder config. The `dir` target does not emit a desktop file.
const path = require('node:path');

const appId = 'io.github.vrcx_team.VRCX';

exports.default = async ({ outDir, packager }) => {
    const { LinuxTargetHelper } = require(require.resolve(
        'app-builder-lib/out/targets/LinuxTargetHelper',
        { paths: [packager.projectDir] }
    ));
    const linux = packager.platformSpecificBuildOptions;
    const options = {
        ...linux,
        category: 'Utility',
        desktop: {
            ...linux.desktop,
            entry: {
                ...linux.desktop?.entry,
                Icon: appId
            }
        }
    };

    await new LinuxTargetHelper(packager).writeDesktopEntry(
        options,
        'vrcx %u',
        path.join(outDir, `${appId}.desktop`)
    );
};
