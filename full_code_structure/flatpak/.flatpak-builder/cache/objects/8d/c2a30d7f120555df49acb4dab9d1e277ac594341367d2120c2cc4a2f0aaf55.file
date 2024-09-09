import time

def output(text, colour, delay=0.01):
    text = colourify(colour) + text
    text = wrap_text(text)
    for char in text:
        print(char, end='', flush=True)
        time.sleep(delay)
    print(colourify("clear"))

def colourify(colour):
    colours = {
        "black": "\033[0;30m",
        "red": "\033[0;31m",
        "green": "\033[0;32m",
        "yellow": "\033[0;33m",
        "blue": "\033[0;34m",
        "magenta": "\033[0;35m",
        "cyan": "\033[0;36m",
        "white": "\033[0;37m",
        "bright_black": "\033[0;90m",
        "bright_red": "\033[0;91m",
        "bright_green": "\033[0;92m",
        "bright_yellow": "\033[0;93m",
        "bright_blue": "\033[0;94m",
        "bright_magenta": "\033[0;95m",
        "bright_cyan": "\033[0;96m",
        "bright_white": "\033[0;97m",
        "clear": "\033[0m",
        "orange": "\033[38;2;255;165;0m",
        "bold_pink": "\033[1m\033[38;2;255;105;180m"
    }
    return colours[colour]

def wrap_text(text, line_length=160):
    lines = []
    while len(text) > line_length:
        wrap_pos = text.rfind(' ', 0, line_length)
        if wrap_pos == -1:
            wrap_pos = line_length
        lines.append(text[:wrap_pos])
        text = text[wrap_pos:].lstrip()
    lines.append(text)
    return "\n".join(lines)
