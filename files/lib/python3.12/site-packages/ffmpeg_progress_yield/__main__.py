import argparse
import platform

from . import __version__ as version
from .ffmpeg_progress_yield import FfmpegProgress


def main() -> None:
    parser = argparse.ArgumentParser(
        description=f"ffmpeg-progress-yield v{version}",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "-d",
        "--duration",
        type=float,
        help="Duration of the video in seconds (override).",
    )
    parser.add_argument(
        "-n", "--dry-run", action="store_true", help="Print ffmpeg command and exit."
    )
    parser.add_argument(
        "-p",
        "--progress-only",
        action="store_true",
        help="Print progress only and do not print stderr at exit.",
    )
    parser.add_argument(
        "-x",
        "--exclude-progress",
        action="store_true",
        help="Exclude progress lines from ffmpeg log.",
    )
    parser.add_argument(
        "-l",
        "--log-file",
        type=str,
        help="Send ffmpeg log output to specified file.",
    )
    parser.add_argument(
        "ffmpeg_command",
        type=str,
        nargs=argparse.REMAINDER,
        help="Any ffmpeg command. Do not quote this argument.",
    )
    args = parser.parse_args()

    ff = FfmpegProgress(args.ffmpeg_command, dry_run=args.dry_run, exclude_progress=args.exclude_progress)

    try:
        from tqdm import tqdm

        with tqdm(
            total=100,
            position=1,
            desc="Progress",
            bar_format="{desc}: {percentage:3.2f}% |{bar}{r_bar}",
        ) as pbar:
            for progress in ff.run_command_with_progress(
                duration_override=args.duration
            ):
                pbar.update(progress - pbar.n)
    except ImportError:
        for progress in ff.run_command_with_progress():
            print(f"\x1b[K{progress}/100", end="\r")
        print()

    if platform.system() == "Windows":
        print("\x1b[K", end="")

    if not args.progress_only:
        log_file = args.log_file
        if log_file in ("1", "2"):
            log_file = int(log_file)
        elif log_file is None:
            log_file = 2
        with open(log_file, "w") as f:
            print(ff.stderr, file=f)


if __name__ == "__main__":
    main()
