## Building

> Note: First make the script executable: `chmod +x tool/build_extension.sh`

### With DDC (for development):

```
./tool/build_extension.sh
```

- The DDC-compiled extension will be located in the `/dev_build/web` directory.

### With dart2js (for release):

```
./tool/build_extension.sh prod
```

- The dart2js-compiled extension will be located in the `/prod_build` directory.

## Local Development

### \[For Googlers\] Create an `extension_key.txt` file:

- Create a `extension_key.txt` file at the root of `/debug_extension`. Paste in
  the value of one of the whitelisted developer keys into this txt file.
  IMPORTANT: DO NOT COMMIT THE KEY. It will be copied into the `manifest.json`
  when you build the extension.

### Build and upload your local extension

- Build the extension following the instructions above
- Visit chrome://extensions
- Toggle "Developer mode" on
- Click the "Load unpacked" button
- Select the extension directory: `dev_build/web`

### Debug your local extension

- Click the Extensions puzzle piece, and pin the Dart Debug Extension with the
  dev icon (unpin the published version so you don't confuse them)
- You can now use the extension normally by clicking it when a local Dart web
  application has loaded in a Chrome tab
- To debug, visit chrome://extensions and click "Inspect view on background
  page" to open Chrome DevTools for the extension
- More debugging information can be found in the
  [Chrome Developers documentation](https://developer.chrome.com/docs/extensions/mv3/devguide/)

## Release process

1. Update the version in `web/manifest.json`, `pubspec.yaml`, and in the
   `CHANGELOG`.
1. Follow the instructions above to build the dart2js-compiled release version
   of the extension.

> \*At this point, you should manually verify that everything is working by
> following the steps in [Local Development](#local-development), except load
> the extension from the `prod_build` directory. You will need to add an
> extension key to the `manifest.json` file in `prod_build` to test locally.

3. Open a PR to submit the version change.
1. Once submitted, pull the changes down to your local branch, and create a zip
   of the `prod_build` directory (NOT `dev_build/web`). **Remove the Googler
   extension key that was added by the builder to the `manifest.json` file.**
1. Rename the zip `version_XX.XX.XX.zip` (eg, `version_1.24.0.zip`) and add it
   to the go/dart-debug-extension-zips folder

> *You must be a Googler to do this. Ask for help if not.*

6. Go to the
   [Chrome Web Store Developer Dashboard](https://chrome.google.com/webstore/devconsole).
1. At the top-right, under Publisher, select dart-bat.

> *If you don’t see dart-bat as an option, you will need someone on the Dart
> team to add you to the dart-bat Google group.*

7. Under Items, select the "Dart Debug Extension".
1. Go to “Package” then select “Upload new package”.

> *The first time you do this, you will be asked to pay a $5 registration fee.
> The registration fee can be expensed.*

9. Upload the zip file you created in step 4.
1. Save as draft, and verify that the new version is correct.
1. Publish. The extension will be published immediately after going through the
   review process.

## Rollback process

> The Chrome Web Store Developer Dashboard does not support rollbacks. Instead
> you must re-publish an earlier version. This means that the extension will
> still have to go through the review process, which can take anywhere from a
> few hours (most common) to a few days.

1. Find the previous version you want to rollback to in the
   go/dart-debug-extension-zips folder.

> > *You must be a Googler to do this. Ask for help if not.*

2. Unzip the version you have chosen, and in `manifest.json` edit the version
   number to be the next sequential version after the current "bad" version (eg,
   the bad version is `1.28.0` and you are rolling back to version `1.27.0`.
   Therefore you change `1.27.0` to `1.29.0`).
1. Re-zip the directory and rename it to the new version number. Add it to the
   go/dart-debug-extension-zips folder.
1. Now, follow steps 6 - 11 in [Release process](#release-process).
