# io.github.slgobinath.SafeEyes

Safe Eyes is a Free and Open Source tool for Linux users to reduce and prevent repetitive strain injury (RSI).

For upstream issues:
    https://github.com/slgobinath/SafeEyes/issues

## Troubleshooting

#### FileNotFoundError: [Errno 2] No such file or directory: ~/.var/app/io.github.slgobinath.SafeEyes/config/safeeyes/safeeyes.json

Safe Eyes expects `safeeyes.json` and `safeeyes_style.css` under XDG_CONFIG_HOME

After running `io.github.slgobinath.SafeEyes` copy and paste this:

```bash
mkdir -p ~/.var/app/io.github.slgobinath.SafeEyes/config/safeeyes/style
touch ~/.var/app/io.github.slgobinath.SafeEyes/config/safeeyes/safeeyes.json
```