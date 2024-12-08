import datetime
import os
import subprocess
import time

import github
from github.GithubException import (
    RateLimitExceededException,
    UnknownObjectException,
    GithubException,
)


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
        # Maintainer EOLs old branches
        "com.riverbankcomputing.PyQt.BaseApp",
        # 19.08 branch is EOL
        "org.freedesktop.LinuxAudio.BaseExtension",
        "org.freedesktop.LinuxAudio.Plugins.LSP",
        "org.freedesktop.LinuxAudio.Plugins.setBfree",
        "org.freedesktop.LinuxAudio.Plugins.sfizz",
        "org.freedesktop.LinuxAudio.Plugins.x42Plugins",
        "org.freedesktop.LinuxAudio.Plugins.ZamPlugins",
        "org.freedesktop.LinuxAudio.Plugins.gmsynth",
        "org.freedesktop.LinuxAudio.Plugins.CAPS",
        "org.freedesktop.LinuxAudio.Plugins.CAPS",
        "org.freedesktop.LinuxAudio.Plugins.VL1Emulator",
        "org.freedesktop.LinuxAudio.Plugins.ZynFusion",
        "org.freedesktop.LinuxAudio.Plugins.CMT",
        "org.freedesktop.LinuxAudio.Plugins.DrumGizmo",
        "org.freedesktop.LinuxAudio.Plugins.TAP",
        "org.freedesktop.LinuxAudio.Plugins.SoSynthLV2",
        "org.freedesktop.LinuxAudio.Plugins.Surge",
        "org.freedesktop.LinuxAudio.Plugins.DPF-Plugins",
        "org.freedesktop.LinuxAudio.Plugins.DISTRHO-Ports",
        "org.freedesktop.LinuxAudio.Plugins.MDA",
        "org.freedesktop.LinuxAudio.Plugins.Ninjas2",
        "org.freedesktop.LinuxAudio.Plugins.reMID-lv2",
        "org.freedesktop.LinuxAudio.Plugins.swh",
        "org.freedesktop.LinuxAudio.Plugins.GxPlugins",
        "org.freedesktop.LinuxAudio.Plugins.KapitonovPluginsPack",
        "org.freedesktop.LinuxAudio.Plugins.ArtyFX",
        "org.freedesktop.LinuxAudio.Plugins.Sorcer",
        "org.freedesktop.LinuxAudio.Plugins.ADLplug",
        "org.freedesktop.LinuxAudio.Plugins.Fabla",
        # 1.6 branch is EOL
        "org.freedesktop.Sdk.Extension.golang",
        # 19.08, 20.08, 21.08 are EOL
        "org.freedesktop.Sdk.Extension.mono6",
        # 1.6, 18.08, 19.08 are EOL
        "org.freedesktop.Sdk.Extension.openjdk8",
        # 18.08 is EOL
        "org.freedesktop.Sdk.Extension.php73",
        # 1.6 is EOL
        "org.freedesktop.Sdk.Extension.rust-stable",
        # 19.08, 20.08 are EOL
        "org.freedesktop.Sdk.Extension.ziglang",
        # master branch is EOL
        "org.freedesktop.Platform.VulkanLayer.vkBasalt",
        # renamed, previous name is EOL but github repo is
        # case-insensitive
        "org.gnome.Chess",
        "org.gnome.GHex",
        "org.gnome.Quadrapassel",
        "org.gnome.Tetravex",
        # EOL-ed to hide from website
        "org.kde.kate",
        # renamed, previous name is EOL but github repo is
        # case-insensitive
        "org.kekikakademi.eFatura",
        # EOL notice added for special reasons
        "org.mozilla.firefox.BaseApp",
        # renamed, previous name is EOL but github repo is
        # case-insensitive
        "org.tabos.twofun",
        # 3-18.08 is EOL
        "org.videolan.VLC.Plugin.bdj",
        # 3-1.6, 3-18.08 is EOL
        "org.videolan.VLC.Plugin.fdkaac",
    }

    stable = get_eol_refs("x86_64", "flathub") | get_eol_refs("aarch64", "flathub")
    beta = get_eol_refs("x86_64", "flathub-beta") | get_eol_refs(
        "aarch64", "flathub-beta"
    )
    eols = list((stable | beta) - excludes)

    if not eols:
        return

    earliest = datetime.datetime.now(datetime.timezone.utc) - datetime.timedelta(
        weeks=60
    )
    count = 0
    while count < len(eols):
        refname = eols[count]
        try:
            repo = g.get_repo(f"flathub/{refname}")
            if not repo.archived:
                try:
                    default_branch = repo.default_branch
                    branch = repo.get_branch(default_branch)
                    last_commit = repo.get_commit(branch.commit.sha)
                    last_commit_time = last_commit.commit.committer.date.astimezone(
                        datetime.timezone.utc
                    )
                except GithubException:
                    last_commit_time = datetime.datetime.now(
                        datetime.timezone.utc
                    ) + datetime.timedelta(seconds=10)
                    pass
                if last_commit_time < earliest:
                    print(
                        "Archiving: {} Repo is in EOL list. Last push: {}, earlier than: {}".format(
                            repo.html_url,
                            last_commit_time.isoformat(),
                            earliest.isoformat(),
                        )
                    )
                    desc = "This repo is archived by Flathub as the app is EOL. If this was done in error, please open an issue at https://github.com/flathub/flathub/issues"
                    repo.edit(description=desc)
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
