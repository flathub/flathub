#!/bin/sh
#
# Runs one command string on the host through the Flatpak portal.
#
# This is the wrapper $CLAUDE_CODE_SHELL_PREFIX points at.
#
# The Claude Code agent picks the shell for its Bash tool by name. It takes
# $CLAUDE_CODE_SHELL or $SHELL only when that path contains "bash" or "zsh",
# and otherwise searches /bin, /usr/bin and /usr/local/bin. Inside the sandbox
# every one of those answers is the runtime's own bash, which has no node, no
# python and none of the user's project tools. Pointing $SHELL at
# /app/bin/host-shell does not reach this code path, because the name is
# neither bash nor zsh.
#
# The agent has a second lever for exactly this case. It wraps every Bash tool
# command in $CLAUDE_CODE_SHELL_PREFIX and hands the whole command over as a
# single argument. So the outer shell stays the runtime's bash and does nothing
# but call this script, and the command itself runs on the host.
#
# host-spawn starts the process on the host, so PATH is the real one. It passes
# on only the variables named by --env. It inherits the working directory,
# which is what keeps "cd" correct from one tool call to the next: the agent
# reads the directory back out of a file under TMPDIR, and TMPDIR is a path
# both sides see.
[ $# -gt 0 ] || exit 0

exec /app/bin/host-spawn --no-pty \
    --env=TERM,COLORTERM,COLUMNS,LINES,LANG,LC_ALL,TMPDIR,TMPPREFIX,CLAUDECODE,CLAUDE_CODE_TMPDIR,CLAUDE_CODE_SESSION_ID,CLAUDE_CODE_ENTRYPOINT,CLAUDE_PROJECT_DIR,CLAUDE_CONFIG_DIR,SSH_AUTH_SOCK,GIT_ASKPASS,SSH_ASKPASS,GIT_TERMINAL_PROMPT \
    /bin/sh -c 'shell=${SHELL:-}
case "$shell" in
    *bash|*zsh) ;;
    *) shell=/bin/bash ;;
esac
[ -x "$shell" ] || shell=/bin/sh
exec "$shell" -c "$1"' host-exec "$@"
