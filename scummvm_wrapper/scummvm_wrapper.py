import configparser
import contextlib
import os
import sys

BASE_CONFIG_NAME = "scummvm.ini"
CONFIG_ENCODING = "utf-8"
SCUMMVM_BINARY = "/app/bin/scummvm"
MUSIC_DRIVER = "fluidsynth"
SOUNDFONT_PATH = "/app/share/soundfonts/FluidR3_GM.sf2"


def get_default_config_file_name():
    home = os.getenv("HOME")
    config_home = os.getenv("XDG_CONFIG_HOME")
    if not config_home:
        if not home:
            raise RuntimeError("environment variable HOME is not set")
        config_home = os.path.join(home, ".config")
    return os.path.join(config_home, "scummvm", BASE_CONFIG_NAME)


def configure_settings(config):
    modified = False
    if not config.has_section("scummvm"):
        config.add_section("scummvm")
        config.set("scummvm", "soundfont", SOUNDFONT_PATH)
        config.set("scummvm", "music_driver", MUSIC_DRIVER)
        config.set("scummvm", "aspect_ratio", "true")
        modified = True
    return modified


def main():
    config_file = get_default_config_file_name()
    with contextlib.suppress(FileExistsError):
        os.makedirs(os.path.dirname(config_file))
    config = configparser.RawConfigParser()
    config.read(config_file, encoding=CONFIG_ENCODING)
    config_modified = configure_settings(config)
    if config_modified:
        with open(config_file, "w", encoding=CONFIG_ENCODING) as f:
            config.write(f)
    os.execve(SCUMMVM_BINARY, [SCUMMVM_BINARY] + sys.argv[1:], os.environ)


if __name__ == "__main__":
    main()
