# org.freedesktop.Sdk.Extension.php84-full

This extension adds PHP 8.4 support to Flatpak.

PHP installs to `/usr/lib/sdk/php84` inside the sandbox.

Example Visual Studio Code Configuration (`settings.json`):

```json
{
  "php.validate.executablePath": "/usr/lib/sdk/php84/bin/php",
  "php.executablePath": "/usr/lib/sdk/php84/bin/php",
}
```

Includes

* [php](https://php.net/)
* [composer](https://github.com/composer/composer)
* [PHIVE](https://phar.io/)
* [apcu](https://pecl.php.net/package/APCu)
* [redis](https://pecl.php.net/package/redis)
* [xdebug](https://xdebug.org/)

Each Flatpak can have its own custom php configuration files.
e.g. for Visual Studio Code
`~/.var/app/com.visualstudio.code/config/php/8.4/ini/my-custom.ini` or `/var/config/php/8.4/ini/my-custom.ini` from a sandboxed shell.

Global composer installs are limited to the Flatpak they were installed in.

## Troubleshooting

### php: No such file or directory

`/usr/bin/env: ‘php’: No such file or directory`

Run `. /usr/lib/sdk/php84/enable.sh` or add `/usr/lib/sdk/php84/bin` to your $PATH.

### Usage with Laravel Extension

To use the [Laravel extension](https://marketplace.visualstudio.com/items?itemName=laravel.vscode-laravel), update `settings.json` to use the sandbox php executable path:

```json
{
    "Laravel.phpCommand": "/usr/lib/sdk/php84/bin/php",
}
```

## Modules

```bash
bash-5.0$ php -m
[PHP Modules]
apcu
bz2
Core
ctype
curl
date
dom
fileinfo
filter
hash
iconv
intl
json
libxml
mbstring
openssl
pcntl
pcre
PDO
pdo_sqlite
Phar
posix
random
redis
Reflection
session
SimpleXML
SPL
sqlite3
standard
tokenizer
xdebug
xml
xmlreader
xmlwriter
Zend OPcache
zip
zlib

[Zend Modules]
Xdebug
Zend OPcache
```

## Build

```bash
flatpak-builder --repo repo .build org.freedesktop.Sdk.Extension.php84.json --force-clean
```
