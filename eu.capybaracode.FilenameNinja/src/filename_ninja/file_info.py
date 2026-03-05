import logging
import os
import stat
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path

log = logging.getLogger("filename_ninja.file_info")


@dataclass
class FileInfo:
    """
    Represents a filesystem entry used by the application.

    Attributes:
        is_folder: True if this entry is a folder, False for files.
        path: The full path to the entry (kept as str for Qt API compatibility).
        filename: The entry name **without** its final extension (no folder components).
            Example: for "archive.tar.gz" this is "archive.tar".
            For files without an extension (or dotfiles like ".env"), this may contain dots.
        suffix: File extension without the leading dot; empty for folders/files without extension.
        depth: Number of path separators for grouping/sorting; computed from `path`.
        proposed_name: Name proposed by renaming rules; defaults to `filename`.
        proposed_suffix: Extension proposed by renaming rules; defaults to `suffix`.
    """
    is_folder: bool
    path: str
    filename: str
    suffix: str = ""
    depth: int = field(init=False)
    proposed_name: str = field(init=False)
    proposed_suffix: str = field(init=False)

    # Extra filesystem metadata (computed best-effort).
    size_bytes: int | None = field(init=False, default=None)
    permissions: str = field(init=False, default="")
    file_type: str = field(init=False, default="")
    created_dt: datetime | None = field(init=False, default=None)
    modified_dt: datetime | None = field(init=False, default=None)
    accessed_dt: datetime | None = field(init=False, default=None)

    def __post_init__(self) -> None:
        """Normalize name/ext invariants and compute derived fields.

        Invariants:
        - For folders: suffix == "".
        - For files: filename is the stem (no final extension), suffix is the final extension (no dot).

        This prevents UI and rename logic from duplicating extensions (e.g. "name.txt.txt").
        """

        # Normalize suffix and ensure `filename` does not include the final extension.
        if self.is_folder:
            self.suffix = ""
        else:
            name = str(self.filename or "")
            base, ext = os.path.splitext(name)

            # If caller passed a full name (with extension), split it.
            if ext:
                self.filename = base
                self.suffix = ext[1:]
            else:
                # Caller likely passed stem + explicit suffix.
                self.suffix = str(self.suffix or "")

            # Defensive: never store a leading dot in suffix.
            if self.suffix.startswith("."):
                self.suffix = self.suffix[1:]
        self.depth = self.path.count(os.path.sep)
        self.proposed_name = self.filename
        self.proposed_suffix = self.suffix

        # Populate metadata for display in the table.
        self._compute_fs_metadata()

    def _compute_fs_metadata(self) -> None:
        """Compute extra filesystem metadata.

        Notes:
        - Uses `os.stat(..., follow_symlinks=False)` to preserve symlink info.
        - "created" time is platform-dependent. On Linux it is typically the
          inode change time (ctime), not true creation time.
        """

        try:
            st = os.stat(self.path, follow_symlinks=False)
        except (FileNotFoundError, PermissionError, OSError):
            self.size_bytes = None
            self.permissions = ""
            self.file_type = ""
            self.created_dt = None
            self.modified_dt = None
            self.accessed_dt = None
            return

        # Size
        self.size_bytes = int(st.st_size)

        # Permissions like "-rw-r--r--".
        try:
            self.permissions = stat.filemode(st.st_mode)
        except Exception:
            log.debug("Failed to get file permissions", exc_info=True)
            self.permissions = ""

        # Type
        if stat.S_ISDIR(st.st_mode):
            self.file_type = "Folder"
        elif stat.S_ISLNK(st.st_mode):
            self.file_type = "Symlink"
        elif stat.S_ISREG(st.st_mode):
            self.file_type = "File"
        else:
            self.file_type = "Other"

        def _dt(ts: float) -> datetime:
            return datetime.fromtimestamp(ts, tz=timezone.utc)

        self.modified_dt = _dt(float(st.st_mtime))
        self.accessed_dt = _dt(float(st.st_atime))
        # Best-effort; see docstring.
        self.created_dt = _dt(float(st.st_ctime))

    @staticmethod
    def _format_dt(dt: datetime | None) -> str:
        if dt is None:
            return ""
        # Keep compact and sortable.
        return dt.astimezone(timezone.utc).strftime("%Y-%m-%d %H:%M:%S")

    @property
    def created_str(self) -> str:
        return self._format_dt(self.created_dt)

    @property
    def modified_str(self) -> str:
        return self._format_dt(self.modified_dt)

    @property
    def accessed_str(self) -> str:
        return self._format_dt(self.accessed_dt)

    @property
    def path_obj(self) -> Path:
        """Return a pathlib.Path view of the stored string path."""
        return Path(self.path)

    @property
    def parent_path(self) -> str:
        """Folder portion of :attr:`path` (no final name component).

        Examples:
        - "/a/b/c.txt" -> "/a/b"
        - "/a/b/dir" -> "/a/b"
        - "/a" -> "/" (platform root)
        """

        try:
            p = self.path_obj
            parent = p.parent
            # Keep as string for Qt model display/sorting.
            return str(parent)
        except Exception:
            log.debug("Failed to resolve parent folder", exc_info=True)
            return ""

    def update_proposed_name(self, proposed_name: str) -> None:
        """Update the proposed (preview) name for this entry."""
        self.proposed_name = proposed_name
