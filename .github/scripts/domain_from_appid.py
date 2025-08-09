# ONLY handle appids that need to use `org.flathub.VerifiedApps.txt`

import sys
import re
from publicsuffixlist import PublicSuffixList

LOGINS = (
    "com.github.",
    "com.gitlab.",
    "io.github.",
    "io.gitlab.",
    "org.gnome.gitlab.",
    "org.gnome.World.",
    "org.gnome.design",
    "org.kde.",
    "org.gnome.",
)


def demangle(name: str) -> str:
    if name.startswith("_"):
        name = name[1:]
    return name.replace("_", "-")


def get_domain(appid: str) -> str:
    if appid.startswith(LOGINS) or appid.count(".") < 2:
        return "None"
    elif appid.startswith(
        ("io.frama.", "page.codeberg.", "io.sourceforge.", "net.sourceforge.")
    ):
        tld, domain, name = appid.split(".")[0:3]
        name = demangle(name)
        if domain == "sourceforge":
            return f"{name}.{domain}.io".lower()
        else:
            return f"{name}.{domain}.{tld}".lower()
    elif appid.startswith(("io.sourceforge.", "net.sourceforge.")):
        [tld, domain, projectname] = appid.split(".")[0:3]
        projectname = demangle(projectname)
        return f"{projectname}.{domain}.io".lower()
    else:
        fqdn = ".".join(reversed(appid.split("."))).lower()
        psl = PublicSuffixList()
        if psl.is_private(fqdn):
            return demangle(psl.privatesuffix(fqdn))
        else:
            return ".".join(
                reversed([demangle(i) for i in appid.split(".")[:-1]])
            ).lower()


if __name__ == "__main__":
    # PR title as input "(Aa)dd com.foo.bar"
    appid = re.sub(r"^\s*add\s+", "", sys.argv[1], flags=re.IGNORECASE)
    print(get_domain(appid).strip())
