import re
import os


def validate_title(title: str) -> int:
    prefix_pattern = r"(?i)^add\s+"
    matched = re.match(prefix_pattern, title)
    if not matched:
        return 42

    appid = title[matched.end() :].strip()
    regex = r"^[A-Za-z_][\w\-]*$"
    split = appid.split(".")

    if (
        len(split) > 255
        or len(split) < 3
        or not all(re.match(regex, sp) for sp in split)
    ):
        return 42

    return 0


if __name__ == "__main__":
    title = os.environ["PR_TITLE"]
    exit_code = validate_title(title)
    print(f"EXIT_CODE={exit_code}")
    exit(0)
