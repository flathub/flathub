# com.github.slgobinath.SafeEyes

Safe Eyes is a Free and Open Source tool for Linux users to reduce and prevent repetitive strain injury (RSI).

For upstream issues:
    https://github.com/slgobinath/SafeEyes/issues

## Troubleshooting

#### FileNotFoundError: [Errno 2] No such file or directory: ~/.var/app/com.github.slgobinath.SafeEyes/config/safeeyes/safeeyes.json

Safe Eyes expects `safeeyes.json` and `safeeyes_style.css` under XDG_DATA_HOME

After running `com.github.slgobinath.SafeEyes` copy and paste this:

```bash
mkdir -p ~/.var/app/com.github.slgobinath.SafeEyes/config/safeeyes/style
touch ~/.var/app/com.github.slgobinath.SafeEyes/config/safeeyes/safeeyes.json
```