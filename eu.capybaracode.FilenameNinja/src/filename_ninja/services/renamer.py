from __future__ import annotations

import re
import unicodedata
from pathlib import Path
from typing import List

from ..file_info import FileInfo
from ..config import CaseChange, ExtConfig, MainConfig, NameConfig, TextTransformConfig
from ..logging_config import get_logger

log = get_logger("renamer")


def _apply_case_change(text: str, how: CaseChange) -> str:
    if how == CaseChange.UPPERCASE:
        return text.upper()
    if how == CaseChange.LOWERCASE:
        return text.lower()
    if how == CaseChange.EVERY_FIRST_LETTER_UP:
        # Capitalize the first letter of every "word".  A word start is defined
        # as a letter preceded by a non-letter (or at the start of the string).
        # Pure string iteration avoids regex edge-cases with special characters.
        lowered = text.lower()
        chars = list(lowered)
        prev_is_letter = False
        for i, ch in enumerate(chars):
            if ch.isalpha():
                if not prev_is_letter:
                    chars[i] = ch.upper()
                prev_is_letter = True
            else:
                prev_is_letter = False
        return "".join(chars)
    if how == CaseChange.ONLY_FIRST_LETTER_UP:
        # Capitalize the first *letter* in the string and lowercase the rest.
        # Non-letter leading characters (e.g. underscores, digits) are preserved.
        lowered = text.lower()
        for i, ch in enumerate(lowered):
            if ch.isalpha():
                return lowered[:i] + ch.upper() + lowered[i + 1:]
        return lowered
    return text


def _remove_accents(text: str) -> str:
    # Normalize and strip combining marks
    normalized = unicodedata.normalize("NFKD", text)
    return "".join(ch for ch in normalized if not unicodedata.combining(ch))


def _apply_text_transform_rules(text: str, rules: TextTransformConfig) -> str:
    """Apply the text-transform rules from a :class:`TextTransformConfig` to *text*.

    This single implementation is used for both the filename-name part and the
    extension part — the logic is identical; only the config prefix differs.
    """
    out = text

    # Pre-processing rules:
    #   1) Remove accents
    #   2) Remove non-alphanumeric (keeping configured allowed characters)
    if getattr(rules, "remove_accents", False):
        out = _remove_accents(out)
    if getattr(rules, "remove_non_alphanumeric", False):
        keep = set(getattr(rules, "keep_these_non_alphanumeric", ""))
        out = "".join(ch for ch in out if ch.isalnum() or ch in keep)

    # Replace
    if rules.is_active_text_to_replace and rules.text_to_replace:
        if getattr(rules, "text_to_replace_case_sensitive", True):
            out = out.replace(rules.text_to_replace, rules.text_for_replacing)
        else:
            # Case-insensitive literal replacement (no regex semantics).
            # Python's str.replace has no ignore-case option, so use a regex with
            # re.escape to treat the needle as a literal.
            out = re.sub(
                re.escape(rules.text_to_replace),
                lambda _m: rules.text_for_replacing,
                out,
                flags=re.IGNORECASE,
            )

    # Crops
    if rules.is_active_left_crop_n_characters and rules.left_crop_n_characters > 0:
        out = out[rules.left_crop_n_characters:]
    if rules.is_active_right_crop_n_characters and rules.right_crop_n_characters > 0:
        out = out[: max(0, len(out) - rules.right_crop_n_characters)]
    if rules.is_active_crop_at_position and rules.crop_how_many > 0:
        pos = max(0, min(len(out), rules.crop_at_position))
        out = out[:pos] + out[pos + rules.crop_how_many :]

    # Inserts
    if rules.is_active_insert_before and rules.insert_before:
        out = rules.insert_before + out
    if rules.is_active_insert_after and rules.insert_after:
        out = out + rules.insert_after
    if rules.is_active_insert_at_position and rules.insert_what:
        pos = max(0, min(len(out), rules.insert_at_position))
        out = out[:pos] + rules.insert_what + out[pos:]

    # Case change — applied after all other text-transform rules so that it
    # operates on the final text (but still before automatic numbering).
    if getattr(rules, "is_active_case_change", False):
        out = _apply_case_change(out, rules.case_change)

    return out


def _apply_regex_rules(full_base_name: str, rules: MainConfig) -> str:
    """Apply optional regex rename to the *base name* (filename without extension).

    When enabled, this runs after Name/Extension rules, so it can restructure the
    final name as a whole.
    """

    # Feature gate: hidden option must be enabled in Settings.
    if not bool(getattr(rules.app, "enable_regex_rename_option", False)):
        return full_base_name

    rx_cfg = getattr(rules, "regex", None)
    if rx_cfg is None:
        return full_base_name

    if not bool(getattr(rx_cfg, "is_active_regex_rename", False)):
        return full_base_name

    pattern = str(getattr(rx_cfg, "regex_pattern", "") or "")
    repl = str(getattr(rx_cfg, "regex_replacement", "") or "")

    # Empty pattern is treated as invalid; main_window blocks rename, but preview
    # should remain stable.
    if not pattern.strip():
        return full_base_name

    try:
        compiled = re.compile(pattern)
    except re.error as e:
        log.debug("Regex compile failed (pattern=%r): %s", pattern, e)
        return full_base_name

    try:
        return compiled.sub(repl, full_base_name)
    except (re.error, IndexError) as e:
        # Replacement backrefs can raise `re.error`, and named-group issues
        # (e.g. "unknown group name") can surface as `IndexError`.
        log.debug("Regex substitution failed (pattern=%r, repl=%r): %s", pattern, repl, e)
        return full_base_name


def preview(entries: List[FileInfo], rules: MainConfig) -> List[FileInfo]:
    """
    Compute preview names based on the provided rules.

    This function mutates each [`FileInfo`](src/filename_ninja/file_info.py:7) in `entries` by
    updating its `proposed_name` and `proposed_suffix` attributes, and returns the same list for
    convenience.

    Numbering is intentionally not applied in this step; it will be added in a later migration step.
    """
    # Numbering should only apply to entries that will actually be renamed.
    # In particular: when folder renaming is disabled, folders must not
    # receive proposed name changes (including numbering).
    #
    # When scanning recursively, numbering may either be continuous across all
    # folders, or restarted per folder, controlled by:
    #   rules.numbering.new_numbering_for_each_folder
    # This option is relevant only when subfolders are loaded.
    numbering_idx = 0
    numbering_idx_by_folder: dict[str, int] = {}

    for _row_idx, fi in enumerate(entries):
        # Decide whether this entry is eligible for preview changes.
        # Files are always eligible; folders depend on config.
        can_rename_this = (not fi.is_folder) or bool(getattr(rules.app, "rename_folders", False))

        if not can_rename_this:
            # Keep folders unchanged in preview when folder renaming is disabled.
            fi.proposed_name = fi.filename
            fi.proposed_suffix = ""
            continue

        if fi.is_folder:
            # For folders, there is no extension. Apply name rules only.
            base = _apply_text_transform_rules(fi.filename, rules.name)
            base = _apply_regex_rules(base, rules)
            fi.proposed_name = base
            fi.proposed_suffix = ""
        else:
            base = _apply_text_transform_rules(fi.filename, rules.name)
            fi.proposed_suffix = _apply_text_transform_rules(fi.suffix, rules.ext)

            # Regex rule applies to the base name (not extension).
            base = _apply_regex_rules(base, rules)
            fi.proposed_name = base

        # Automatic numbering (optional). Applied after other rules.
        if getattr(rules.numbering, "is_active_auto_number_change", False):
            how = rules.numbering.auto_number_change

            # Keep numbering stable for preview by using the sequence of *rename-eligible*
            # entries (folders excluded when their renaming is disabled).
            #
            # If "New numbering for each folder" is enabled AND the entries were
            # loaded from subfolders, restart numbering for each parent folder.
            if bool(getattr(rules.app, "load_subfolders", False)) and bool(
                getattr(rules.numbering, "new_numbering_for_each_folder", False)
            ):
                folder_key = str(Path(fi.path).parent)
                folder_idx = int(numbering_idx_by_folder.get(folder_key, 0))
                number = int(rules.numbering.start_with) + (folder_idx * int(rules.numbering.increment_by))
                numbering_idx_by_folder[folder_key] = folder_idx + 1
            else:
                number = int(rules.numbering.start_with) + (numbering_idx * int(rules.numbering.increment_by))
            s = str(number)
            z = int(rules.numbering.zero_fill_how_many)
            if z > 0:
                s = s.zfill(z)

            if how == getattr(type(how), "BEFORE_FILENAME"):
                fi.proposed_name = s + fi.proposed_name
            elif how == getattr(type(how), "AFTER_FILENAME"):
                fi.proposed_name = fi.proposed_name + s
            elif how == getattr(type(how), "BEFORE_EXTENSION"):
                # Treat as part of the extension string.
                fi.proposed_suffix = s + fi.proposed_suffix
            elif how == getattr(type(how), "AFTER_EXTENSION"):
                fi.proposed_suffix = fi.proposed_suffix + s

        numbering_idx += 1

    return entries
