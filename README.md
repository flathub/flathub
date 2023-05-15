# org.freedesktop.Sdk.Extension.php82

This extension adds PHP support to Flatpak.

PHP installs to `/usr/lib/sdk/php82` inside the sandbox.

Example Visual Studio Code Configuration

```json
"php.validate.executablePath": "/usr/lib/sdk/php82/bin/php",
"php.executablePath": "/usr/lib/sdk/php82/bin/php",
```

Includes

* [php](https://php.net/)
* [composer](https://github.com/composer/composer)
* [PHIVE](https://phar.io/)
* [apcu](https://pecl.php.net/package/APCu)
* [xdebug](https://xdebug.org/)

Each Flatpak can have its own custom php configuration files.
e.g. for Visual Studio Code
`~/.var/app/com.visualstudio.code/config/php/8.2/ini/my-custom.ini` or `/var/config/php/8.2/ini/my-custom.ini` from a sandboxed shell.

Global composer installs are limited to the Flatpak they were installed in.

#### Troubleshooting
`/usr/bin/env: ‘php’: No such file or directory`

Run `. /usr/lib/sdk/php82/enable.sh` or add `/usr/lib/sdk/php82/bin` to your $PATH.

#### Modules

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
zip
zlib

[Zend Modules]
Xdebug
```
#### Build
```bash
flatpak-builder --repo repo .build org.freedesktop.Sdk.Extension.php82.json --force-clean
```
