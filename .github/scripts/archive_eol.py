import os
import subprocess
import time

import github
from github.GithubException import RateLimitExceededException, UnknownObjectException


def ignore_ref(ref: str) -> bool:
    ref_splits = ref.split("/")

    if len(ref_splits) != 4:
        return True

    if ref_splits[2] not in ("x86_64", "aarch64") or ref_splits[1].endswith(
        (".Debug", ".Locale", ".Sources")
    ):
        return True
    return False


def get_eol_refs(arch: str, remote: str) -> set:
    cmd = (
        f'flatpak remote-ls --user --arch="{arch}" --all --columns=ref,options {remote}'
    )
    ret = subprocess.run(cmd, shell=True, capture_output=True, encoding="utf-8")
    eol_refs: set = set()

    if ret.returncode == 0 and ret.stdout:
        for line in ret.stdout.split("\n"):
            if line:
                split = line.split("\t")
            if len(split) < 2:
                continue
            ref, options = split
            if ignore_ref(ref):
                continue
            refname = ref.split("/")[1]
            if any(x in options for x in ("eol=", "eol-rebase=")):
                eol_refs.add(refname)

    return eol_refs


def main() -> None:

    token = os.environ["GITHUB_TOKEN"]

    g = github.Github(auth=github.Auth.Token(token))

    # Exclude refs that have EOL notices but are still maintained
    # in some branch
    excludes = {
        "com.riverbankcomputing.PyQt.BaseApp",
        "org.freedesktop.LinuxAudio.BaseExtension",
        "org.freedesktop.LinuxAudio.Plugins.LSP",
        "org.freedesktop.LinuxAudio.Plugins.setBfree",
        "org.freedesktop.LinuxAudio.Plugins.sfizz",
        "org.freedesktop.LinuxAudio.Plugins.x42Plugins",
        "org.freedesktop.LinuxAudio.Plugins.ZamPlugins",
        "org.freedesktop.Sdk.Extension.golang",
        "org.freedesktop.Sdk.Extension.mono6",
        "org.freedesktop.Sdk.Extension.openjdk8",
        "org.freedesktop.Sdk.Extension.php73",
        "org.freedesktop.Sdk.Extension.rust-stable",
        "org.freedesktop.Sdk.Extension.ziglang",
        "org.gnome.Chess",
        "org.gnome.GHex",
        "org.gnome.Quadrapassel",
        "org.gnome.Tetravex",
        "org.kde.kate",
        "org.kekikakademi.eFatura",
        "org.mozilla.firefox.BaseApp",
        "org.tabos.twofun",
        "org.videolan.VLC.Plugin.bdj",
        "org.videolan.VLC.Plugin.fdkaac",
    }
    # we need to get apps which are EOL on both supported arches
    # as there are apps which are only EOL on one arch but maintained
    # in another. This might miss some refs but LBYL!
    stable = get_eol_refs("x86_64", "flathub") & get_eol_refs("aarch64", "flathub")
    eols = list(stable - excludes)

    if not eols:
        return

    earliest = datetime.datetime.now(datetime.UTC) - datetime.timedelta(weeks=60)
    count = 0
    while count < len(eols):
        refname = eols[count]
        try:
            repo = g.get_repo(f"flathub/{refname}")
            if not repo.archived:
                print("Archiving {}".format(repo.html_url))
                repo.edit(archived=True)
        except UnknownObjectException:
            pass
        except RateLimitExceededException:
            print("Rate limited")
            time.sleep(g.rate_limiting_resettime - time.time() + 10)
            continue
        count += 1


if __name__ == "__main__":
    main()
