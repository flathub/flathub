if [ ! -f "${XDG_CONFIG_HOME}/yazi/theme.toml" ]; then
    install -Dm644 "/app/share/theme-no-nerd-fonts.toml" "${XDG_CONFIG_HOME}/yazi/theme.toml"
fi
yazi %u
