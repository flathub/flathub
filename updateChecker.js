const https = require('https');
const { URL } = require('url');
const { dialog, shell } = require('electron');

const latestReleaseUrl = 'https://github.com/A-Star100/simpliplay-desktop/releases/latest/';

function fetchRedirectLocation(url) {
  return new Promise((resolve, reject) => {
    https.get(url, (res) => {
      if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
        resolve(res.headers.location);
      } else {
        reject(new Error(`Expected redirect but got status code: ${res.statusCode}`));
      }
    }).on('error', reject);
  });
}

function normalizeVersion(version) {
  return version.trim().replace(/[^\d\.]/g, '');
}

function compareVersions(v1, v2) {
  const a = v1.split('.').map(Number);
  const b = v2.split('.').map(Number);
  const len = Math.max(a.length, b.length);

  for (let i = 0; i < len; i++) {
    const num1 = a[i] || 0;
    const num2 = b[i] || 0;
    if (num1 > num2) return 1;
    if (num1 < num2) return -1;
  }
  return 0;
}

/**
 * Checks for update and shows native dialog if update is available.
 * @param {string} currentVersion 
 */
async function checkForUpdate(currentVersion) {
  try {
    const redirectUrl = await fetchRedirectLocation(latestReleaseUrl);

    const urlObj = new URL(redirectUrl);
    const parts = urlObj.pathname.split('/');

    const releaseTag = parts[parts.length - 1];
    const versionMatch = releaseTag.match(/release-([\d\.]+)/);
    if (!versionMatch) {
      throw new Error(`Could not parse version from release tag: ${releaseTag}`);
    }
    const latestVersion = normalizeVersion(versionMatch[1]);

    const cmp = compareVersions(latestVersion, currentVersion);

    if (cmp > 0) {
      const result = dialog.showMessageBoxSync({
        type: 'info',
        buttons: ['Download', 'Later'],
        defaultId: 0,
        cancelId: 1,
        title: 'Update Available',
        message: `A new version (${latestVersion}) is available. Would you like to download it?`,
      });

      if (result === 0) {
        shell.openExternal("https://simpliplay.netlify.app/#download-options");
      }
    } else {
      dialog.showMessageBoxSync({
        type: 'info',
        buttons: ['OK'],
        title: "You're up to date!",
        message: `You are using the latest version (${currentVersion}).`,
      });
    }
  } catch (err) {
    dialog.showErrorBox('Could not check for update.', err.message);
  }
}

module.exports = { checkForUpdate };
