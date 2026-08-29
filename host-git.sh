#!/bin/sh
#
# Runs git on the host through the Flatpak portal.
#
# Claude Code Desktop builds its own search path, looks for a program literally
# named "git" on it, and spawns that. The runtime has no git. Installing one in
# the sandbox would only silence the check, because every git command would
# still run against a sandbox that holds none of the user's tools and none of
# the hooks in their repositories. So /app/bin/git and /app/bin/git-lfs are
# symlinks to this script, and the real git on the host does the work, in the
# same directory, on the same files.
#
# host-spawn passes on only the variables named below. Everything else the host
# git needs is already in place, because the process starts on the host.
cmd=${0##*/}
if [ "$cmd" = "host-git" ]; then
    cmd=git
fi

exec /app/bin/host-spawn \
    --env=TERM,LANG,LC_ALL,SSH_AUTH_SOCK,GIT_ASKPASS,SSH_ASKPASS,GIT_SSH_COMMAND,GIT_TERMINAL_PROMPT,GIT_CONFIG_GLOBAL,GIT_CONFIG_SYSTEM,GIT_DIR,GIT_WORK_TREE,GIT_INDEX_FILE,GIT_LFS_SKIP_SMUDGE,GCM_INTERACTIVE \
    "$cmd" "$@"
