import sys
from publicsuffixlist import PublicSuffixList

CODE_HOSTS = (
    "io.github.",
    "io.frama.",
    "io.gitlab.",
    "page.codeberg.",
    "io.sourceforge.",
    "net.sourceforge.",
    "org.gnome.gitlab.",
    "org.freedesktop.gitlab.",
    "site.srht.",
)

def demangle(name: str) -> str:
    if name.startswith("_"):
        name = name[1:]
    return name.replace("_", "-")

def get_domain(appid: str) -> str | None:
    if appid.startswith(CODE_HOSTS) or appid.count(".") < 2:
        return None

    if appid.startswith("org.gnome.") and not appid.startswith("org.gnome.gitlab."):
        return "gnome.org"
    elif appid.startswith("org.kde."):
        return "kde.org"
    elif appid.startswith("org.freedesktop.") and not appid.startswith("org.freedesktop.gitlab."):
        return "freedesktop.org"
    else:
        fqdn = ".".join(reversed(appid.split("."))).lower()
        psl = PublicSuffixList()
        if psl.is_private(fqdn):
            return demangle(psl.privatesuffix(fqdn))
        else:
            return ".".join(reversed([demangle(i) for i in appid.split(".")[:-1]])).lower()

if __name__ == "__main__":
    appid = sys.argv[1]
    print(get_domain(appid))
