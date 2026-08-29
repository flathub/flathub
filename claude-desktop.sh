#!/bin/bash
#
# Launcher used inside the Flatpak sandbox.
#
# zypak-wrapper comes from org.electronjs.Electron2.BaseApp. It intercepts
# Chromium's zygote and sandbox calls and maps them onto the Flatpak sandbox,
# so the app keeps a real renderer sandbox without the setuid chrome-sandbox
# helper, and without --no-sandbox.
#
# Electron writes scratch files, including the Claude Code terminal's, into
# TMPDIR. Point it at the per-app runtime directory so nothing leaks into a
# shared /tmp.
set -e

export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"
mkdir -p "${TMPDIR}"

# Claude Code has to run on the host, with the user's real shell, PATH and
# tools. Two variables get it there, and they cover different code paths.
#
# $SHELL is what the app itself reads: the worker that resolves the login-shell
# environment, and the Claude Code terminal. Point it at the wrapper that hands
# both to the host through the org.freedesktop.Flatpak portal.
#
# $CLAUDE_CODE_SHELL_PREFIX is what the agent reads. The agent picks its own
# shell by name and only accepts a path containing bash or zsh, so $SHELL never
# reaches it and its Bash tool would run in the sandbox. It does wrap every one
# of those commands in this prefix, so the prefix is the way in.
#
# Probe the portal first. If it does not answer, leave both alone, so the app
# falls back to its old sandbox-only behaviour instead of failing to open a
# terminal at all.
if /app/bin/host-spawn --no-pty true >/dev/null 2>&1; then
    export SHELL=/app/bin/host-shell
    export CLAUDE_CODE_SHELL_PREFIX=/app/bin/host-exec
else
    echo "claude-desktop: host portal unreachable, Claude Code stays in the sandbox" >&2
fi

exec zypak-wrapper /app/extra/claude-desktop "$@"
