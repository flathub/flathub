#!/bin/sh
#
# Stand-in for the user's shell inside the sandbox.
#
# Claude Code Desktop reads $SHELL twice. Once to learn the environment: it runs
# "$SHELL" -l -i -c 'env' in a worker and keeps the PATH that comes back. Once
# to open the Claude Code terminal: it runs "$SHELL" -l in a pty. Inside the
# sandbox both land on the runtime's own bash, which has no git, no node and
# none of the user's project tools. This script sends them to the host instead.
#
# host-spawn passes on only the variables named by --env, so the host process
# starts from the host session environment. That is the point: the PATH that
# comes back is the real one. $SHELL on the host is the host's own shell, so
# there is no loop back into this script.
exec /app/bin/host-spawn --env=TERM,COLORTERM \
    /bin/sh -c 'exec "${SHELL:-/bin/sh}" "$@"' host-shell "$@"
