#!/usr/bin/env bash
set -e

CWD=$(pwd)

podman run --rm --privileged \
    -v $HOME:$HOME:rslave \
    -v $CWD:$CWD:rslave \
    -w $CWD \
    -e GIT_AUTHOR_NAME="$GIT_USER_NAME" \
    -e GIT_COMMITTER_NAME="$GIT_USER_NAME" \
    -e GIT_AUTHOR_EMAIL="$GIT_USER_EMAIL" \
    -e GIT_COMMITTER_EMAIL="$GIT_USER_EMAIL" \
    -it ghcr.io/flathub/flatpak-external-data-checker \
    info.portfolio_performance.PortfolioPerformance.json

