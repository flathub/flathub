from dataclasses import dataclass
from pathlib import Path
from typing import List, Iterable, Callable, Optional
import fnmatch
import os

from ..file_info import FileInfo
from ..logging_config import get_logger


log = get_logger("scanner")


@dataclass
class ScanOptions:
    mask: str = "*"
    include_folders: bool = True
    include_files: bool = True
    recursive: bool = False
    case_sensitive: bool = False


def _parse_patterns(mask: str) -> List[str]:
    if not mask:
        return ["*"]
    parts = [p.strip() for p in mask.replace(",", ";").split(";")]
    return [p for p in parts if p] or ["*"]


def _match_name(name: str, patterns: Iterable[str], case_sensitive: bool) -> bool:
    if case_sensitive:
        return any(fnmatch.fnmatchcase(name, pat) for pat in patterns)
    low_name = name.lower()
    return any(fnmatch.fnmatchcase(low_name, pat.lower()) for pat in patterns)


def scan_folder(
    base: Path,
    options: ScanOptions,
    *,
    should_cancel: Optional[Callable[[], bool]] = None,
    on_progress: Optional[Callable[[int], None]] = None,
) -> List[FileInfo]:
    """
    Scan a folder and return a sorted list of FileInfo entries.
    - Folders are listed regardless of mask when include_folders is True.
    - Files are filtered by the given mask(s) when include_files is True.
    Sorting: folders by name; files by (suffix, name), case-insensitive.
    """
    base = Path(base)
    results: List[FileInfo] = []
    if not base.exists() or not base.is_dir():
        log.warning("Scan skipped: base path is not a folder: %s", str(base))
        return results
    patterns = _parse_patterns(options.mask)

    log.info(
        "Scan start: base=%s mask=%s recursive=%s include_folders=%s include_files=%s case_sensitive=%s",
        str(base),
        options.mask,
        bool(options.recursive),
        bool(options.include_folders),
        bool(options.include_files),
        bool(options.case_sensitive),
    )

    scanned_count = 0

    def handle_entry(p: Path) -> None:
        nonlocal scanned_count
        if should_cancel and should_cancel():
            raise StopIteration()
        try:
            scanned_count += 1
            if on_progress:
                on_progress(scanned_count)
            if p.is_dir():
                if options.include_folders and (p.parent == base or options.recursive):
                    results.append(FileInfo(True, str(p), p.name, ""))
            else:
                if not options.include_files:
                    return
                if _match_name(p.name, patterns, options.case_sensitive):
                    # suffix without dot
                    suf = p.suffix[1:] if p.suffix.startswith(".") else p.suffix
                    results.append(FileInfo(False, str(p), p.name, suf))
        except PermissionError:
            log.warning("Scan permission denied: %s", str(p))
        except OSError as e:
            # Best-effort scanning: skip entries that error at stat/is_dir/etc.
            log.warning("Scan OS error: %s (%s)", str(p), str(e))

    try:
        if options.recursive:
            for p in base.rglob("*"):
                handle_entry(p)
        else:
            for p in base.iterdir():
                handle_entry(p)
    except StopIteration:
        # Cancellation requested; return partial results to caller (caller may discard).
        log.info("Scan canceled: base=%s scanned=%d", str(base), int(scanned_count))
        return results

    log.info("Scan finished: base=%s scanned=%d results=%d", str(base), int(scanned_count), int(len(results)))

    # Sort: folders first by name, then files by (suffix, name)
    dirs = [fi for fi in results if fi.is_folder]
    files = [fi for fi in results if not fi.is_folder]
    dirs.sort(key=lambda d: d.filename.lower())
    files.sort(key=lambda f: (f.suffix.lower(), f.filename.lower()))
    return dirs + files
