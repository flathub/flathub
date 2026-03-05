from __future__ import annotations

from typing import List, Optional

from PySide6.QtCore import QAbstractTableModel, QModelIndex, Qt, QFileInfo
from PySide6.QtWidgets import QFileIconProvider

from ..file_info import FileInfo


class RenameTableModel(QAbstractTableModel):
    """Single model that contains both *original* and *proposed* file name parts.

    This model intentionally exposes 4 columns so that two different table views can
    share the same model (and thus the same row order / sorting), while each view
    simply hides the columns it doesn't want to show.

    Columns:
      0 - Original name (with icon via DecorationRole)
      1 - Original extension
      2 - Proposed name (with icon via DecorationRole)
      3 - Proposed extension
    """

    ORIG_NAME_COL = 0
    ORIG_EXT_COL = 1
    NEW_NAME_COL = 2
    NEW_EXT_COL = 3

    # Metadata columns (shown in current table; hidden in preview table)
    SIZE_COL = 4
    TYPE_COL = 5
    PERMS_COL = 6
    PATH_COL = 7
    CREATED_COL = 8
    MODIFIED_COL = 9
    ACCESSED_COL = 10

    HEADERS = [
        "Name",
        "Extension",
        "New Name Preview",
        "New Extension Preview",
        "Size",
        "Type",
        "Permissions",
        "Path",
        "Created",
        "Modified",
        "Accessed",
    ]

    def __init__(self, items: Optional[List[FileInfo]] = None, parent=None) -> None:
        super().__init__(parent)
        self._items: List[FileInfo] = list(items) if items else []
        self._icons = QFileIconProvider()

    # -- Qt model required overrides --

    def rowCount(self, parent: QModelIndex = QModelIndex()) -> int:  # type: ignore[override]
        if parent.isValid():
            return 0
        return len(self._items)

    def columnCount(self, parent: QModelIndex = QModelIndex()) -> int:  # type: ignore[override]
        if parent.isValid():
            return 0
        return len(self.HEADERS)

    def data(self, index: QModelIndex, role: int = Qt.ItemDataRole.DisplayRole):  # type: ignore[override]
        if not index.isValid():
            return None

        row = index.row()
        col = index.column()

        if row < 0 or row >= len(self._items):
            return None

        item = self._items[row]

        if role == Qt.ItemDataRole.DisplayRole:
            if col == self.ORIG_NAME_COL:
                return item.filename
            if col == self.ORIG_EXT_COL:
                return item.suffix
            if col == self.NEW_NAME_COL:
                return item.proposed_name
            if col == self.NEW_EXT_COL:
                return item.proposed_suffix
            if col == self.SIZE_COL:
                if item.is_folder:
                    return ""
                return "" if item.size_bytes is None else str(int(item.size_bytes))
            if col == self.TYPE_COL:
                return str(item.file_type or ("Folder" if item.is_folder else "File"))
            if col == self.PERMS_COL:
                return str(item.permissions or "")
            if col == self.PATH_COL:
                return str(item.parent_path)
            if col == self.CREATED_COL:
                return str(item.created_str)
            if col == self.MODIFIED_COL:
                return str(item.modified_str)
            if col == self.ACCESSED_COL:
                return str(item.accessed_str)

        if role == Qt.ItemDataRole.DecorationRole and col in (self.ORIG_NAME_COL, self.NEW_NAME_COL):
            if item.is_folder:
                return self._icons.icon(QFileIconProvider.IconType.Folder)
            # Use original path for icon resolution
            return self._icons.icon(QFileInfo(item.path))

        if role == Qt.ItemDataRole.TextAlignmentRole:
            if col == self.SIZE_COL:
                return int(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
            return int(Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter)

        return None

    def headerData(self, section: int, orientation: Qt.Orientation, role: int = Qt.ItemDataRole.DisplayRole):  # type: ignore[override]
        if role != Qt.ItemDataRole.DisplayRole:
            return None
        if orientation == Qt.Orientation.Horizontal:
            if 0 <= section < len(self.HEADERS):
                return self.HEADERS[section]
            return None
        return str(section + 1)

    def flags(self, index: QModelIndex) -> Qt.ItemFlags:  # type: ignore[override]
        if not index.isValid():
            return Qt.ItemFlag.NoItemFlags
        return Qt.ItemFlag.ItemIsEnabled | Qt.ItemFlag.ItemIsSelectable

    # -- Data management API --

    def set_items(self, items: List[FileInfo]) -> None:
        self.beginResetModel()
        self._items = list(items)
        self.endResetModel()

    def items(self) -> List[FileInfo]:
        """Return the current items in row order.

        Note: returns the internal list by reference for performance.
        Callers should treat it as read-only.
        """

        return self._items

    def get_item(self, row: int) -> Optional[FileInfo]:
        if 0 <= row < len(self._items):
            return self._items[row]
        return None

    def sort(self, column: int, order: Qt.SortOrder = Qt.SortOrder.AscendingOrder) -> None:  # type: ignore[override]
        """Sort rows, keeping folders grouped first regardless of sort order."""

        reverse = order == Qt.SortOrder.DescendingOrder

        def norm(s: str) -> str:
            return (s or "").lower()

        if column == self.ORIG_NAME_COL:
            key_func = lambda it: (norm(it.filename), norm(it.suffix))
        elif column == self.ORIG_EXT_COL:
            key_func = lambda it: (norm(it.suffix), norm(it.filename))
        elif column == self.NEW_NAME_COL:
            key_func = lambda it: (norm(it.proposed_name), norm(it.proposed_suffix))
        elif column == self.NEW_EXT_COL:
            key_func = lambda it: (norm(it.proposed_suffix), norm(it.proposed_name))
        elif column == self.SIZE_COL:
            key_func = lambda it: (-1 if it.is_folder else int(it.size_bytes or 0), norm(it.filename), norm(it.suffix))
        elif column == self.TYPE_COL:
            key_func = lambda it: (norm(it.file_type), norm(it.filename), norm(it.suffix))
        elif column == self.PERMS_COL:
            key_func = lambda it: (norm(it.permissions), norm(it.filename), norm(it.suffix))
        elif column == self.PATH_COL:
            key_func = lambda it: (norm(it.parent_path), norm(it.filename), norm(it.suffix))
        elif column == self.CREATED_COL:
            key_func = lambda it: (it.created_dt.timestamp() if it.created_dt else 0.0, norm(it.filename), norm(it.suffix))
        elif column == self.MODIFIED_COL:
            key_func = lambda it: (it.modified_dt.timestamp() if it.modified_dt else 0.0, norm(it.filename), norm(it.suffix))
        elif column == self.ACCESSED_COL:
            key_func = lambda it: (it.accessed_dt.timestamp() if it.accessed_dt else 0.0, norm(it.filename), norm(it.suffix))
        else:
            return

        self.layoutAboutToBeChanged.emit()
        self._items.sort(key=key_func, reverse=reverse)
        # Always keep folders first (stable sort).
        self._items.sort(key=lambda it: not it.is_folder)
        self.layoutChanged.emit()

    def notify_proposed_changed(self) -> None:
        """Notify views that the proposed (preview) columns have changed.

        This is used when renaming rules are edited and we recompute `proposed_*`
        in-place for the existing row set.
        """

        if not self._items:
            return

        top_left = self.index(0, self.NEW_NAME_COL)
        bottom_right = self.index(len(self._items) - 1, self.NEW_EXT_COL)
        self.dataChanged.emit(
            top_left,
            bottom_right,
            [
                Qt.ItemDataRole.DisplayRole,
            ],
        )
