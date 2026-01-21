Source code for SimpliPlay.

--BUILD STUFF--
SimpliPlay uses Electron Builder for desktop builds, but includes an example Electron Forge config that will work as expected but does not support native actions such as file picking from the explorer; the package.json uses electron-builder specific actions for that.

--CI/CD--
We now use CI/CD (specifically GitHub Actions, at one point CircleCI) for builds, but Linux ARM64 snaps in particular were annoying to get started with. After multiple attempts, it finally worked as expected (with a few really stupid workarounds required that shouldn't even be needed).

--DEPENDENCIES--
Other than Electron and Electron Builder, there is one more dependency called "Nothing".
Its name is very self explanatory, isn't it?
