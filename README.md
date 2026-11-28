# NonBlockingLife on Flathub

## Introduction

We are usually interrupted while working on tasks, making it easy to lose track of what we were doing. NonBlockingLife helps you note the current moment, handle the inserted tasks, and more easily return to your main personal tasks and time management.

This is the Flathub packaging repository for [NonBlockingLife](https://github.com/ychsue/NonBlockingLife) and you can read its [README](https://github.com/ychsue/NonBlockingLife/blob/main/README.md) to get more information about it.

## Building Locally

To build, install, and lint the Flatpak locally:

```bash
# Build and install locally
flatpak run --command=flathub-build org.flatpak.Builder --install io.github.ychsue.nonblockinglife.yml

# Run the application
flatpak run io.github.ychsue.nonblockinglife

# Run Flathub linter checks
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest io.github.ychsue.nonblockinglife.yml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo
```

## Testing

For someone who doesn't have aarch64 hardware, you can test it through Azure's aarch64 VM. The needed scripts and instructions are in the [NonBlockingLife repository](https://github.com/ychsue/NonBlockingLife/tree/main/desktop/flatpak-aarch64-test) and [the discussion with Gemini](https://github.com/ychsue/NonBlockingLife/blob/main/Discussion/cross_platform/discussion_20261126_rent_aarch64.md).