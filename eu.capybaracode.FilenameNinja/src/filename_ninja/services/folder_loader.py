from __future__ import annotations

import time
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

from PySide6 import QtCore

from ..config import MainConfig
from ..file_info import FileInfo
from ..logging_config import get_logger
from .scanner import ScanOptions, scan_folder
from .renamer import preview


log = get_logger("folder_loader")

@dataclass(frozen=True)
class LoadRequest:
    base_path: Path
    options: ScanOptions
    rules: MainConfig


class _FolderLoadWorker(QtCore.QObject):
    """Worker that runs filesystem scan + preview generation in a background thread."""

    progress = QtCore.Signal(int)  # scanned_count
    finished = QtCore.Signal(list)  # List[FileInfo] (entries with proposed_* filled)
    failed = QtCore.Signal(str)
    canceled = QtCore.Signal()

    def __init__(self, request: LoadRequest) -> None:
        super().__init__()
        self._request = request
        self._cancel = False

        # Emit throttling to avoid flooding the UI thread
        self._scanned_count = 0
        self._last_progress_emit = 0.0

    def cancel(self) -> None:
        self._cancel = True

    def _should_cancel(self) -> bool:
        return self._cancel

    def _on_progress(self, scanned_count: int) -> None:
        # Emit at most ~10 times/sec; the dialog is indeterminate anyway,
        # but this is useful for future status text/logging.
        now = time.monotonic()
        if now - self._last_progress_emit >= 0.1:
            self._last_progress_emit = now
            self.progress.emit(scanned_count)

    @QtCore.Slot()
    def run(self) -> None:
        try:
            log.info(
                "Folder load job start: base=%s recursive=%s include_folders=%s include_files=%s mask=%s",
                str(self._request.base_path),
                bool(self._request.options.recursive),
                bool(self._request.options.include_folders),
                bool(self._request.options.include_files),
                str(self._request.options.mask),
            )
            file_infos: list[FileInfo] = scan_folder(
                self._request.base_path,
                self._request.options,
                should_cancel=self._should_cancel,
                on_progress=self._on_progress,
            )

            if self._should_cancel():
                log.info("Folder load job canceled after scan: base=%s", str(self._request.base_path))
                self.canceled.emit()
                return

            # Mutates file_infos in-place by filling proposed_* fields.
            preview(file_infos, self._request.rules)

            if self._should_cancel():
                log.info("Folder load job canceled after preview: base=%s", str(self._request.base_path))
                self.canceled.emit()
                return

            log.info(
                "Folder load job finished: base=%s entries=%d",
                str(self._request.base_path),
                int(len(file_infos)),
            )
            self.finished.emit(file_infos)
        except Exception as e:  # pragma: no cover (UI will show details)
            log.exception(
                "Folder load job failed: base=%s error=%s",
                str(getattr(self._request, "base_path", "")),
                str(e),
            )
            self.failed.emit(str(e))


class FolderLoadController(QtCore.QObject):
    """Qt-aware orchestration service for (path -> scan -> preview) background jobs."""

    load_started = QtCore.Signal(int)  # job_id
    load_progress = QtCore.Signal(int, int)  # job_id, scanned_count
    load_finished = QtCore.Signal(int, list)  # job_id, file_infos (with proposed_* filled)
    load_failed = QtCore.Signal(int, str)  # job_id, error_text
    load_canceled = QtCore.Signal(int)  # job_id

    def __init__(self, parent: Optional[QtCore.QObject] = None) -> None:
        super().__init__(parent)
        self._next_job_id = 0
        self._active_job_id: Optional[int] = None

        self._thread: Optional[QtCore.QThread] = None
        self._worker: Optional[_FolderLoadWorker] = None

    @property
    def active_job_id(self) -> Optional[int]:
        return self._active_job_id

    def load(self, request: LoadRequest) -> int:
        """Start a new load job; cancels any in-flight job."""

        self.cancel_current()

        self._next_job_id += 1
        job_id = self._next_job_id
        self._active_job_id = job_id

        log.info("Folder load scheduled: job_id=%d base=%s", int(job_id), str(request.base_path))

        thread = QtCore.QThread(self)
        worker = _FolderLoadWorker(request)
        worker.moveToThread(thread)

        # Wire worker -> controller (include job_id)
        worker.progress.connect(lambda scanned: self.load_progress.emit(job_id, scanned))
        worker.finished.connect(lambda files: self._on_finished(job_id, files))
        worker.failed.connect(lambda err: self._on_failed(job_id, err))
        worker.canceled.connect(lambda: self._on_canceled(job_id))

        # Lifecycle
        thread.started.connect(worker.run)
        worker.finished.connect(thread.quit)
        worker.failed.connect(thread.quit)
        worker.canceled.connect(thread.quit)
        thread.finished.connect(worker.deleteLater)
        thread.finished.connect(thread.deleteLater)

        # Store refs
        self._thread = thread
        self._worker = worker

        self.load_started.emit(job_id)
        thread.start()
        return job_id

    def cancel_current(self) -> None:
        if self._worker is not None:
            log.info("Folder load cancel requested: job_id=%s", str(self._active_job_id))
            self._worker.cancel()

    def cancel(self, job_id: int) -> None:
        if self._active_job_id == job_id:
            self.cancel_current()

    def _on_finished(self, job_id: int, files: list[FileInfo]) -> None:
        if self._active_job_id == job_id:
            self._active_job_id = None
        self.load_finished.emit(job_id, files)

    def _on_failed(self, job_id: int, error_text: str) -> None:
        if self._active_job_id == job_id:
            self._active_job_id = None
        self.load_failed.emit(job_id, error_text)

    def _on_canceled(self, job_id: int) -> None:
        if self._active_job_id == job_id:
            self._active_job_id = None
        self.load_canceled.emit(job_id)