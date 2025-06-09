#!/usr/bin/env python3
import sys
import os
import base64
from PySide6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QPushButton, QFileDialog,
    QProgressBar, QMessageBox, QTableWidget, QTableWidgetItem, QAbstractItemView, QHBoxLayout, QHeaderView,
    QLineEdit, QLabel, QDialog, QTextEdit, QDialogButtonBox
)
from PySide6.QtGui import QIntValidator, QIcon
from PySide6.QtCore import Qt, QThread, Signal, QObject
from pathlib import Path
import subprocess

# Embed SVG icon as base64 string
MUSEAMP_SVG_BASE64 = """
PHN2ZyB3aWR0aD0iMTBpbiIgaGVpZ2h0PSIxMGluIiB2aWV3Qm94PSIwIDAgNzIwIDcyMCIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIiB4bWxuczp4bGluaz0iaHR0cDovL3d3dy53My5vcmcvMTk5OS94bGluayI+CiA8ZGVmcz4KICA8bGluZWFyR3JhZGllbnQgaWQ9ImEiIHgxPSIyODIuMTEiIHgyPSI1NzYuMTEiIHkxPSIxNjkuNTciIHkyPSIxNjkuNTciIGdyYWRpZW50VW5pdHM9InVzZXJTcGFjZU9uVXNlIj4KICAgPHN0b3Agc3RvcC1jb2xvcj0iI2ZmMDBlMSIgb2Zmc2V0PSIwIi8+CiAgIDxzdG9wIHN0b3AtY29sb3I9IiNlNGU0ZmYiIG9mZnNldD0iLjM5ODUiLz4KICAgPHN0b3Agc3RvcC1jb2xvcj0iI2ZmYmZhYyIgb2Zmc2V0PSIuODg4MyIvPgogIDwvbGluZWFyR3JhZGllbnQ+CiAgPGxpbmVhckdyYWRpZW50IGlkPSJiIiB4MT0iMjAwLjk1IiB4Mj0iMjU5LjMzIiB5MT0iNTM3LjI3IiB5Mj0iNTM3LjI3IiBncmFkaWVudFVuaXRzPSJ1c2VyU3BhY2VPblVzZSI+CiAgIDxzdG9wIHN0b3AtY29sb3I9IiMxYjE0NjQiIG9mZnNldD0iMCIvPgogICA8c3RvcCBzdG9wLWNvbG9yPSIjMWIxNDhmIiBvZmZzZXQ9Ii41MTAyIi8+CiAgIDxzdG9wIHN0b3AtY29sb3I9IiMxYjE0N2IiIG9mZnNldD0iLjg4ODMiLz4KICA8L2xpbmVhckdyYWRpZW50PgogIDxsaW5lYXJHcmFkaWVudCBpZD0iYyIgeDE9IjIwNS4yOCIgeDI9IjMwNi44OSIgeTE9IjQxOS42NyIgeTI9IjQxOS42NyIgeGxpbms6aHJlZj0iI2IiLz4KICA8bGluZWFyR3JhZGllbnQgaWQ9ImQiIHgxPSIxNDEuNSIgeDI9IjI1OC4yNSIgeTE9IjUxMi40NSIgeTI9IjUxMi40NSIgeGxpbms6aHJlZj0iI2IiLz4KICA8bGluZWFyR3JhZGllbnQgaWQ9ImUiIHgxPSI0NjAuMzkiIHgyPSI1NjIiIHkxPSI0MjQuMjQiIHkyPSI0MjQuMjQiIHhsaW5rOmhyZWY9IiNiIi8+CiAgPGxpbmVhckdyYWRpZW50IGlkPSJmIiB4MT0iNDU5LjMyIiB4Mj0iNTE1LjAzIiB5MT0iNTQxLjciIHkyPSI1NDEuNyIgeGxpbms6aHJlZj0iI2IiLz4KICA8bGluZWFyR3JhZGllbnQgaWQ9ImciIHgxPSIzOTguNzciIHgyPSI1MTMuMzYiIHkxPSI1MjEuMDkiIHkyPSI1MjEuMDkiIHhsaW5rOmhyZWY9IiNiIi8+CiAgPGxpbmVhckdyYWRpZW50IGlkPSJoIiB4MT0iMjY1IiB4Mj0iNTc5IiB5MT0iMjUyLjUiIHkyPSIyNTIuNSIgeGxpbms6aHJlZj0iI2IiLz4KICA8bGluZWFyR3JhZGllbnQgaWQ9ImkiIHgxPSIyNzMuNyIgeDI9IjU3OSIgeTE9IjIzNS4xOCIgeTI9IjIzNS4xOCIgZ3JhZGllbnRVbml0cz0idXNlclNwYWNlT25Vc2UiPgogICA8c3RvcCBzdG9wLWNvbG9yPSIjYWYyNzhmIiBvZmZzZXQ9IjAiLz4KICAgPHN0b3Agc3RvcC1jb2xvcj0iI2E2MDA1ZCIgb2Zmc2V0PSIuMjgxNyIvPgogICA8c3RvcCBzdG9wLWNvbG9yPSIjZDExNDVhIiBvZmZzZXQ9Ii44ODgzIi8+CiAgPC9saW5lYXJHcmFkaWVudD4KIDwvZGVmcz4KIDx0aXRsZT5tdXNpY3dhdmU8L3RpdGxlPgogPHBhdGggZD0ibTI4NCAyMjkuNWM1LTE0Ljc0NCAxNC0yNy44NSAyMS00Mi4wNDggNS04LjczNzMgMTAtMTYuMzgyIDE4LTIwLjc1MSA2LTIuMTg0NCA3IDUuNDYwOCA4IDkuODI5NXYzMC41ODFjLTEgNC4zNjg3IDEgMTAuOTIyIDYgMTIuMDE0IDIgMCAzLTIuMTg0MyA0LTMuMjc2NSA0LTkuODI5NSA1LTIwLjc1MSA4LTMwLjU4MSA2LTE3LjQ3NSAxMC0zNi4wNDIgMjEtNTEuMzMyIDQtNC4zNjg3IDkgMS4wOTIxIDEwIDQuMzY4NiAwIDIuMTg0NCAxIDQuMzY4NyAyIDcuNjQ1MiAzIDE2LjM4MiAzIDMyLjc2NSAzIDQ5LjE0OGExMS4zMDMgMTEuMzAzIDAgMCAwIDkgMTAuOTIyYzEgMCAyLTEuMDkyMSAzLTEuMDkyMSA0LTQuMzY4NyA2LTkuODI5NSA4LTE1LjI5IDQtMTcuNDc1IDYtMzMuODU3IDEzLTUwLjI0YTI0LjM4MyAyNC4zODMgMCAwIDAgMy01LjQ2MDhjMS0yLjE4NDMgNC00LjM2ODcgNi0zLjI3NjUgMiAwIDQgNC4zNjg2IDUgNi41NTMgNyAyNS4xMiA0IDUwLjI0IDggNzYuNDUyIDEgNy42NDUyIDEyIDguNzM3MyAxNyAzLjI3NjUgMTEtMTQuMTk4IDExLTMyLjc2NSAxMy01MC4yNCAyLTcuNjQ1MiA1LTE1LjI5IDEyLTE2LjM4MiA0LTEuMDkyMiA2IDQuMzY4NiA4IDcuNjQ1MSA2IDE2LjM4MiA1IDMzLjg1NyA4IDUyLjQyNCAxIDUuNDYwOCA3IDYuNTUzIDEyIDcuNjQ1MiA1IDAgMTAtNC4zNjg3IDEzLTguNzM3NCA2LTE0LjE5OCA3LTI5LjQ4OCAxMS00NC43NzkgNC0xNi4zODIgNS0zMy44NTcgMTMtNDkuMTQ3IDEtMi4xODQ0IDMtNC4zNjg3IDUtNC4zNjg3IDYgNC4zNjg3IDUgMTMuMTA2IDcgMjAuNzUxdjkuODI5NGMzIDI0LjAyOCAyIDQ2Ljk2MyA0IDcwLjk5MSAwIDYuNTUzIDggOC43Mzc0IDExLjQ1IDEzLjkiIGZpbGw9Im5vbmUiIHN0cm9rZT0iIzllMDA1ZCIgc3Ryb2tlLW1pdGVybGltaXQ9IjEwIiBzdHJva2Utd2lkdGg9IjgiLz4KIDxwYXRoIGQ9Im0yODQgMjI5LjVjNS0xNC43NDQgMTQtMjcuODUgMjEtNDIuMDQ4IDUtOC43MzczIDEwLTE2LjM4MiAxOC0yMC43NTEgNi0yLjE4NDQgNyA1LjQ2MDggOCA5LjgyOTV2MzAuNTgxYy0xIDQuMzY4NyAxIDEwLjkyMiA2IDEyLjAxNCAyIDAgMy0yLjE4NDMgNC0zLjI3NjUgNC05LjgyOTUgNS0yMC43NTEgOC0zMC41ODEgNi0xNy40NzUgMTAtMzYuMDQyIDIxLTUxLjMzMiA0LTQuMzY4NyA5IDEuMDkyMSAxMCA0LjM2ODYgMCAyLjE4NDQgMSA0LjM2ODcgMiA3LjY0NTIgMyAxNi4zODIgMyAzMi43NjUgMyA0OS4xNDhhMTEuMzAzIDExLjMwMyAwIDAgMCA5IDEwLjkyMmMxIDAgMi0xLjA5MjEgMy0xLjA5MjEgNC00LjM2ODcgNi05LjgyOTUgOC0xNS4yOSA0LTE3LjQ3NSA2LTMzLjg1NyAxMy01MC4yNGEyNC4zODMgMjQuMzgzIDAgMCAwIDMtNS40NjA4YzEtMi4xODQzIDQtNC4zNjg3IDYtMy4yNzY1IDIgMCA0IDQuMzY4NiA1IDYuNTUzIDcgMjUuMTIgNCA1MC4yNCA4IDc2LjQ1MiAxIDcuNjQ1MiAxMiA4LjczNzMgMTcgMy4yNzY1IDExLTE0LjE5OCAxMS0zMi43NjUgMTMtNTAuMjQgMi03LjY0NTIgNS0xNS4yOSAxMi0xNi4zODIgNC0xLjA5MjIgNiA0LjM2ODYgOCA3LjY0NTEgNiAxNi4zODIgNSAzMy44NTcgOCA1Mi40MjQgMSA1LjQ2MDggNyA2LjU1MyAxMiA3LjY0NTIgNSAwIDEwLTQuMzY4NyAxMy04LjczNzQgNi0xNC4xOTggNy0yOS40ODggMTEtNDQuNzc5IDQtMTYuMzgyIDUtMzMuODU3IDEzLTQ5LjE0NyAxLTIuMTg0NCAzLTQuMzY4NyA1LTQuMzY4NyA2IDQuMzY4NyA1IDEzLjEwNiA3IDIwLjc1MXY5LjgyOTRjMyAyNC4wMjggMiA0Ni45NjMgNCA3MC45OTEgMCA2LjU1MyA4IDguNzM3NCAxMS40NSAxMy45IiBmaWxsPSJub25lIiBzdHJva2U9InVybCgjYSkiIHN0cm9rZS1taXRlcmxpbWl0PSIxMCIgc3Ryb2tlLXdpZHRoPSI0Ii8+CiA8cGF0aCBkPSJtMjU5LjMzIDUxNC4zN3MtNC4zMjM5IDQ5LjcyNS01OC4zNzMgNDcuNTYzYzAgMmUtNCAzMC4yNjctNjAuNTM0IDU4LjM3My00Ny41NjN6IiBmaWxsPSJ1cmwoI2IpIiBzdHJva2U9IiMxYzAwNWUiIHN0cm9rZS1taXRlcmxpbWl0PSIxMCIvPgogPHBhdGggZD0ibTIwOS45OCA1NTUuNjdhNzkuNDQ4IDc5LjQ0OCAwIDAgMSAzMi44MTgtMjMuMjI0IDEzLjMxMSAxMy4zMTEgMCAwIDEgMy4zNTUyLTAuNzk5MmMxLjk5NjItMi45ODcyIDQuMTQtNS45MTU4IDYuMzg1My04Ljc5MWExMC45MzcgMTAuOTM3IDAgMCAxIDYuMDI5LTMuODgyOGMwLjAzMjUtMC4wMzI0IDAuMDY4LTAuMDU4OSAwLjEwMDctMC4wOTA4bDQ4LjIyMy0yMzcuODNoLTQyLjE5M2wtNTkuNDIgMjc3LjIyaDMuMjg2OWE2Ljk2NDUgNi45NjQ1IDAgMCAxIDEuNDE1MS0yLjYxMDh6IiBmaWxsPSJ1cmwoI2cpIiBzdHJva2U9IiMxYzAwNWUiIHN0cm9rZS1taXRlcmxpbWl0PSIxMCIvPgogPGVsbGlwc2UgY3g9IjE5OS44NyIgY3k9IjUxMi40NSIgcng9IjU4LjM3MyIgcnk9IjUzLjQwNSIgZmlsbD0idXJsKCNkKSIgc3Ryb2tlPSIjMWMwMDVlIiBzdHJva2UtbWl0ZXJsaWl0PSIxMCIvPgogPHBvbHlnb24gcG9pbnRzPSI1NjIgMjgyIDU3OSAyMjMgMjgwIDIyMyAyNjUgMjgxIiBmaWxsPSJ1cmwoI2gpIiBzdHJva2U9IiMxYzAwNWUiIHN0cm9rZS1taXRlcmxpbWl0PSIxMCIgc3Ryb2tlLXdpZHRoPSI0LjMiLz4KIDxwb2x5Z29uIHBvaW50cz0iMjczLjcgMjQ3LjM1IDU3MS45OCAyNDcuMzUgNTc5IDIyMyAyODAgMjIzIiBmaWxsPSJ1cmwoI2kpIi8+Cjwvc3ZnPgo=
"""

#supported file types
supported_filetypes = {".flac", ".mp3"}

#class to define gui
class ErrorLogDialog(QDialog):
    def __init__(self, log_text, parent=None):
        #basic window properties
        super().__init__(parent)
        self.setWindowTitle("Error Log")
        self.setMinimumSize(400, 300)
        #layout and text setup
        layout = QVBoxLayout(self)
        self.text_edit = QTextEdit()
        self.text_edit.setReadOnly(True)
        self.text_edit.setText(log_text)
        layout.addWidget(self.text_edit)

        #button setup
        button_box = QDialogButtonBox()
        copy_btn = QPushButton("Copy Log")
        ok_btn = QPushButton("OK")
        button_box.addButton(copy_btn, QDialogButtonBox.ActionRole)
        button_box.addButton(ok_btn, QDialogButtonBox.AcceptRole)
        layout.addWidget(button_box)
        copy_btn.clicked.connect(self.copy_log)
        ok_btn.clicked.connect(self.accept)
    def copy_log(self):
        clipboard = QApplication.clipboard()
        clipboard.setText(self.text_edit.toPlainText())

#worker for background operations
class Worker(QObject):
    finished = Signal(list, list)   #updates, error_logs
    progress = Signal(int)  #percent complete

    def __init__(self, files, lufs=None):
        super().__init__()
        self.files = files
        self.lufs = lufs

    #runner for worker thread
    def run(self):
        updates = []
        error_logs = []
        total = len(self.files)
        processed = 0

        #helper to update progress bar
        def emit_progress():
            percent = int((processed / total) * 100) if total else 100
            if percent > 100:
                percent = 100
            self.progress.emit(percent)

        # Only tag mode remains
        for row, file_path in enumerate(self.files):
            ext = Path(file_path).suffix.lower()
            if ext not in supported_filetypes:
                updates.append((row, "-", "-", "-"))
                processed += 1
                emit_progress()
                continue
            #build rsgain command to apply ReplayGain tag
            #self.lufs is expected to be an int between 5 and 30
            lufs_str = f"-{abs(int(self.lufs))}" if self.lufs is not None else "-18"
            loudness_val = "-"
            replaygain_val = "-"
            clipping_val = "-"
            #use -S if user chose not to overwrite
            rsgain_cmd = [
                "rsgain", "custom", "-s", "i", "-l", lufs_str, "-O", file_path
            ]
            if hasattr(self, "overwrite_rg") and not self.overwrite_rg:
                rsgain_cmd.insert(2, "-S")
            try:
                proc = subprocess.run(
                    rsgain_cmd,
                    capture_output=True, text=True, check=False
                )
                output = proc.stdout
                if proc.returncode == 0:
                    lines = output.strip().splitlines()
                    if len(lines) >= 2:
                        header = lines[0].split('\t')
                        values = lines[1].split('\t')
                        colmap = {k: i for i, k in enumerate(header)}
                        lufs = values[colmap.get("Loudness (LUFS)", -1)] if "Loudness (LUFS)" in colmap else "-"
                        gain = values[colmap.get("Gain (dB)", -1)] if "Gain (dB)" in colmap else "-"
                        if lufs != "-":
                            loudness_val = f"{lufs} LUFS"
                        if gain != "-":
                            replaygain_val = gain
                        #clipping: check "Clipping" or "Clipping Adjustment?" column from rsgain
                        clip_idx = colmap.get("Clipping", colmap.get("Clipping Adjustment?", -1))
                        if clip_idx != -1:
                            clip_val = values[clip_idx]
                            if clip_val.strip().upper() in ("Y", "YES"):
                                clipping_val = "Yes"
                            elif clip_val.strip().upper() in ("N", "NO"):
                                clipping_val = "No"
                            else:
                                clipping_val = clip_val
                else:
                    error_logs.append(f"{file_path}:\n{proc.stderr or proc.stdout}")
            except Exception as e:
                error_logs.append(f"{file_path}: {str(e)}")
            updates.append((row, loudness_val, replaygain_val, clipping_val))
            processed += 1
            emit_progress()
        self.finished.emit(updates, error_logs)

#worker for adding files/folders
class AddFilesWorker(QObject):
    finished = Signal(list, list)   #updates, error_logs
    progress = Signal(int)  #percent complete

    def __init__(self, files):
        super().__init__()
        self.files = files

    #runner for add files/folder worker
    def run(self):
        updates = []
        error_logs = []
        total = len(self.files)
        for idx, file_path in enumerate(self.files):
            path = Path(file_path)
            loudness_val = "-"
            replaygain_val = "-"
            clipping_val = "-"
            #check if file exists
            if not path.is_file():
                error_logs.append(f"{file_path}: Not a file")
                updates.append((idx, loudness_val, replaygain_val, clipping_val))
                self.progress.emit(int((idx + 1) / total * 100))
                continue
            #check if file type is supported (taken from top supported_filetypes var)
            if path.suffix.lower() not in supported_filetypes:
                error_logs.append(f"{file_path}: Unsupported file type")
                updates.append((idx, loudness_val, replaygain_val, clipping_val))
                self.progress.emit(int((idx + 1) / total * 100))
                continue
            try:
                cmd = [
                    "rsgain", "custom",
                    "-O",
                    str(path)
                ]
                proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
                output = proc.stdout
                if proc.returncode == 0:
                    lines = output.strip().splitlines()
                    if len(lines) >= 2:
                        header = lines[0].split('\t')
                        values = lines[1].split('\t')
                        colmap = {k: i for i, k in enumerate(header)}
                        lufs = values[colmap.get("Loudness (LUFS)", -1)] if "Loudness (LUFS)" in colmap else "-"
                        gain = values[colmap.get("Gain (dB)", -1)] if "Gain (dB)" in colmap else "-"
                        if lufs != "-":
                            loudness_val = f"{lufs} LUFS"
                        if gain != "-":
                            replaygain_val = gain
                        #clipping: check "Clipping" or "Clipping Adjustment?" column in rsgain
                        clip_idx = colmap.get("Clipping", colmap.get("Clipping Adjustment?", -1))
                        if clip_idx != -1:
                            clip_val = values[clip_idx]
                            if clip_val.strip().upper() in ("Y", "YES"):
                                clipping_val = "Yes"
                            elif clip_val.strip().upper() in ("N", "NO"):
                                clipping_val = "No"
                            else:
                                clipping_val = clip_val
                else:
                    error_logs.append(f"{file_path}: rsgain failed\n{proc.stderr or proc.stdout}")
            except Exception as e:
                error_logs.append(f"{file_path}: {str(e)}")
            updates.append((idx, loudness_val, replaygain_val, clipping_val))
            self.progress.emit(int((idx + 1) / total * 100))
        self.finished.emit(updates, error_logs)

class ApplyGainWorker(QObject):
    finished = Signal(list, list)  #error_logs, analysis_results
    progress = Signal(int)   #percent

    def __init__(self, files, lufs, table, supported_filetypes):
        super().__init__()
        self.files = files
        self.lufs = lufs
        self.table = table
        self.supported_filetypes = supported_filetypes

    def run(self):
        error_logs = []
        total = len(self.files)
        for idx, file_path in enumerate(self.files):
            ext = Path(file_path).suffix.lower()
            if ext not in self.supported_filetypes:
                continue
            lufs_str = f"-{abs(self.lufs)}"
            #Tag with user LUFS to get gain value
            tag_cmd = [
                "rsgain", "custom", "-s", "i", "-l", lufs_str, "-O", file_path
            ]
            gain_val = None
            try:
                proc_tag = subprocess.run(tag_cmd, capture_output=True, text=True, check=False)
                if proc_tag.returncode != 0:
                    error_logs.append(f"{file_path} (tag):\n{proc_tag.stderr or proc_tag.stdout}")
                    self.progress.emit(int((idx + 1) / total * 100))
                    continue
                output = proc_tag.stdout
                lines = output.strip().splitlines()
                if len(lines) >= 2:
                    header = lines[0].split('\t')
                    values = lines[1].split('\t')
                    colmap = {k: i for i, k in enumerate(header)}
                    gain_idx = colmap.get("Gain (dB)", -1)
                    if gain_idx != -1 and gain_idx < len(values):
                        gain_val = values[gain_idx]
                    else:
                        gain_val = None
            except Exception as e:
                error_logs.append(f"{file_path} (tag): {str(e)}")
                self.progress.emit(int((idx + 1) / total * 100))
                continue

            if gain_val is None or gain_val == "-":
                error_logs.append(f"{file_path}: Could not determine ReplayGain value.")
                self.progress.emit(int((idx + 1) / total * 100))
                continue

            #apply gain using ffmpeg (in-place)
            try:
                gain_db = float(gain_val)
            except Exception:
                error_logs.append(f"{file_path}: Invalid gain value '{gain_val}'.")
                self.progress.emit(int((idx + 1) / total * 100))
                continue

            tmp_file = str(Path(file_path).with_suffix(f".gain_tmp{ext}"))
            ffmpeg_cmd = [
                "ffmpeg", "-y", "-i", file_path,
                "-map_metadata", "0", "-map", "0",
                "-af", f"volume={gain_db}dB",
                "-c:v", "copy"
            ]
            if ext == ".mp3":
                ffmpeg_cmd += ["-c:a", "libmp3lame"]
            elif ext == ".flac":
                ffmpeg_cmd += ["-c:a", "flac"]
                #detect original bit depth of FLAC and preserve it
                try:
                    probe = subprocess.run(
                        ["ffprobe", "-v", "error", "-select_streams", "a:0", "-show_entries", "stream=bits_per_raw_sample,bits_per_sample", "-of", "default=noprint_wrappers=1:nokey=1", file_path],
                        capture_output=True, text=True, check=False
                    )
                    bit_depths = [int(x) for x in probe.stdout.strip().splitlines() if x.isdigit()]
                    if bit_depths:
                        bit_depth = max(bit_depths)
                        #map bit depth to ffmpeg sample_fmt
                        if bit_depth == 16:
                            ffmpeg_cmd += ["-sample_fmt", "s16"]
                        elif bit_depth == 24:
                            ffmpeg_cmd += ["-sample_fmt", "s32"]  # FLAC stores 24-bit as s32
                        elif bit_depth == 32:
                            ffmpeg_cmd += ["-sample_fmt", "s32"]
                        #else: let ffmpeg default
                except Exception:
                    pass
            ffmpeg_cmd.append(tmp_file)

            try:
                proc_ffmpeg = subprocess.run(ffmpeg_cmd, capture_output=True, text=True, check=False)
                if proc_ffmpeg.returncode != 0:
                    error_logs.append(f"{file_path} (ffmpeg):\n{proc_ffmpeg.stderr or proc_ffmpeg.stdout}")
                    if os.path.exists(tmp_file):
                        os.remove(tmp_file)
                    self.progress.emit(int((idx + 1) / total * 100))
                    continue
                os.replace(tmp_file, file_path)
            except Exception as e:
                error_logs.append(f"{file_path} (ffmpeg): {str(e)}")
                if os.path.exists(tmp_file):
                    os.remove(tmp_file)
            self.progress.emit(int((idx + 1) / total * 100))

        #analyze files after gain applied (for table update)
        analysis_results = []
        for idx, file_path in enumerate(self.files):
            loudness_val = "-"
            replaygain_val = "-"
            clipping_val = "-"
            ext = Path(file_path).suffix.lower()
            if ext not in self.supported_filetypes:
                analysis_results.append((idx, loudness_val, replaygain_val, clipping_val))
                continue
            try:
                proc = subprocess.run(
                    ["rsgain", "custom", "-s", "i", "-l", lufs_str, "-O", file_path],
                    capture_output=True, text=True, check=False
                )
                output = proc.stdout
                if proc.returncode == 0:
                    lines = output.strip().splitlines()
                    if len(lines) >= 2:
                        header = lines[0].split('\t')
                        values = lines[1].split('\t')
                        colmap = {k: i for i, k in enumerate(header)}
                        lufs = values[colmap.get("Loudness (LUFS)", -1)] if "Loudness (LUFS)" in colmap else "-"
                        gain = values[colmap.get("Gain (dB)", -1)] if "Gain (dB)" in colmap else "-"
                        if lufs != "-":
                            loudness_val = f"{lufs} LUFS"
                        if gain != "-":
                            replaygain_val = gain
                        clip_idx = colmap.get("Clipping", colmap.get("Clipping Adjustment?", -1))
                        if clip_idx != -1:
                            clip_val = values[clip_idx]
                            if clip_val.strip().upper() in ("Y", "YES"):
                                clipping_val = "Yes"
                            elif clip_val.strip().upper() in ("N", "NO"):
                                clipping_val = "No"
                            else:
                                clipping_val = clip_val
                else:
                    error_logs.append(f"{file_path} (analyze):\n{proc.stderr or proc.stdout}")
            except Exception as e:
                error_logs.append(f"{file_path} (analyze): {str(e)}")
            analysis_results.append((idx, loudness_val, replaygain_val, clipping_val))
        self.finished.emit(error_logs, analysis_results)

class AudioToolGUI(QWidget):
    def __init__(self):
        super().__init__()
        #set basic window properties
        self.setWindowTitle("MuseAmp")
        self.setMinimumSize(700, 500)
        # Decode base64 SVG and set as icon
        svg_bytes = base64.b64decode(MUSEAMP_SVG_BASE64)
        from PySide6.QtSvgWidgets import QSvgWidget
        from PySide6.QtGui import QPixmap
        from PySide6.QtCore import QByteArray
        svg_widget = QSvgWidget()
        svg_widget.load(QByteArray(svg_bytes))
        pixmap = QPixmap(svg_widget.size())
        svg_widget.render(pixmap)
        icon = QIcon(pixmap)
        self.setWindowIcon(icon)
        self.layout = QVBoxLayout(self)  #main vertical layout

        #file info table setup
        self.table = QTableWidget()
        self.table.setColumnCount(5)
        self.table.setHorizontalHeaderLabels(["File Path", "Extension", "File Loudness", "ReplayGain", "Clipping"])
        self.table.setEditTriggers(QAbstractItemView.NoEditTriggers)    #make cells read-only
        self.table.setSelectionBehavior(QAbstractItemView.SelectRows)   #select entire rows
        self.table.horizontalHeader().setSectionResizeMode(QHeaderView.Interactive) #let user resize columns
        self.table.horizontalHeader().setStretchLastSection(True) #stretch last column to fill space

        #buttons
        self.add_files_btn = QPushButton("Add File(s)")
        self.add_folder_btn = QPushButton("Add Folder")
        self.remove_files_btn = QPushButton("Remove File(s)")
        self.gain_btn = QPushButton("Apply Gain")
        self.replaygain_btn = QPushButton("Analyze && Tag")

        #textbox for replaygain value input
        self.replaygain_input = QLineEdit()
        self.replaygain_input.setFixedWidth(50) #fix width for neatness
        self.replaygain_input.setText("18") #default ReplayGain 2.0 LUFS value (positive version)
        self.replaygain_input.setValidator(QIntValidator(5, 30, self))  #allow only 5 to 30

        #label for replaygain input
        self.replaygain_label = QLabel("Target LUFS: -")

        #layout for replaygain label + input
        self.replaygain_layout = QHBoxLayout()
        self.replaygain_layout.addWidget(self.replaygain_label)
        self.replaygain_layout.addWidget(self.replaygain_input)

        #progress bar defaulting to 100
        self.progress_bar = QProgressBar()
        self.progress_bar.setAlignment(Qt.AlignCenter)
        self.progress_bar.setTextVisible(True)
        self.progress_bar.setValue(100)         #default to 100%
        self.progress_bar.setFormat("100%")

        #horizontal layout for buttons and input
        self.button_layout = QHBoxLayout()
        for btn in [
            self.add_files_btn, self.add_folder_btn,
            self.remove_files_btn, self.gain_btn, self.replaygain_btn
        ]:
            self.button_layout.addWidget(btn)

        #add replaygain input layout to the right of replaygain button
        self.button_layout.addLayout(self.replaygain_layout)

        #add widgets to the main layout
        self.layout.addWidget(self.table)
        self.layout.addLayout(self.button_layout)
        self.layout.addWidget(self.progress_bar)
        
        #connect buttons to functions
        self.add_files_btn.clicked.connect(self.add_files)
        self.add_folder_btn.clicked.connect(self.add_folder)
        self.remove_files_btn.clicked.connect(self.remove_files)
        self.replaygain_btn.clicked.connect(self.analyze_and_tag)
        self.gain_btn.clicked.connect(self.apply_gain_adjust)

    #add files to table/list
    def add_files(self):
        files, _ = QFileDialog.getOpenFileNames(self, "Select Files")
        if not files:
            return
        self.set_ui_enabled(False)
        self.set_progress(0)
        files_to_add = []
        for file_path in files:
            if not self.is_already_listed(file_path):
                files_to_add.append(file_path)
        #insert rows now, do not scan yet, just set "-" for columns
        for file_path in files_to_add:
            row = self.table.rowCount()
            self.table.insertRow(row)
            self.table.setItem(row, 0, QTableWidgetItem(str(file_path)))
            self.table.setItem(row, 1, QTableWidgetItem(Path(file_path).suffix.lower()))
            self.table.setItem(row, 2, QTableWidgetItem("-"))
            self.table.setItem(row, 3, QTableWidgetItem("-"))
            self.table.setItem(row, 4, QTableWidgetItem("-"))
        self.set_ui_enabled(True)
        self.set_progress(100)

    #what to do when files are finished being added
    def _on_add_files_finished(self, updates, error_logs, start_row):
        for idx, loudness_val, replaygain_val, clipping_val in updates:
            row = start_row + idx
            self.table.setItem(row, 2, QTableWidgetItem(loudness_val))
            self.table.setItem(row, 3, QTableWidgetItem(replaygain_val))
            self.table.setItem(row, 4, QTableWidgetItem(clipping_val))
        self.set_ui_enabled(True)
        self.set_progress(100)
        if error_logs:
            dlg = ErrorLogDialog("\n\n".join(error_logs), self)
            dlg.exec()

    #add supported files from folder to table/list
    def add_folder(self):
        folder = QFileDialog.getExistingDirectory(self, "Select Folder")
        if not folder:
            return
        self.set_ui_enabled(False)
        self.set_progress(0)
        files_to_add = []
        #check to see if supported file type
        for root, _, files in os.walk(folder):
            for filename in files:
                path = Path(root) / filename
                if path.suffix.lower() in supported_filetypes and not self.is_already_listed(str(path)):
                    files_to_add.append(str(path))
        #insert items into rows now, do not scan yet, just set "-" for columns
        start_row = self.table.rowCount()
        for file_path in files_to_add:
            row = self.table.rowCount()
            self.table.insertRow(row)
            self.table.setItem(row, 0, QTableWidgetItem(str(file_path)))
            self.table.setItem(row, 1, QTableWidgetItem(Path(file_path).suffix.lower()))
            self.table.setItem(row, 2, QTableWidgetItem("-"))
            self.table.setItem(row, 3, QTableWidgetItem("-"))
            self.table.setItem(row, 4, QTableWidgetItem("-"))
        self.set_ui_enabled(True)
        self.set_progress(100)

    #actually add file to the table/list (used for single file add)
    def add_file_to_table(self, file_path):
        path = Path(file_path)
        if not path.is_file():
            return
        if self.is_already_listed(str(path)):
            return

        row = self.table.rowCount()
        self.table.insertRow(row)
        self.table.setItem(row, 0, QTableWidgetItem(str(path))) #file path
        self.table.setItem(row, 1, QTableWidgetItem(path.suffix.lower()))   #extension

        #scan for existing ReplayGain tags
        loudness_val = "-"
        replaygain_val = "-"
        clipping_val = "-"
        try:
            cmd = [
                "rsgain", "custom",
                "-O",
                str(path)
            ]
            proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
            output = proc.stdout
            if proc.returncode == 0:
                lines = output.strip().splitlines()
                if len(lines) >= 2:
                    header = lines[0].split('\t')
                    values = lines[1].split('\t')
                    colmap = {k: i for i, k in enumerate(header)}
                    lufs = values[colmap.get("Loudness (LUFS)", -1)] if "Loudness (LUFS)" in colmap else "-"
                    gain = values[colmap.get("Gain (dB)", -1)] if "Gain (dB)" in colmap else "-"
                    if lufs != "-":
                        loudness_val = f"{lufs} LUFS"
                    if gain != "-":
                        replaygain_val = gain
                    #clipping: check "Clipping" or "Clipping Adjustment?" column from rsgain
                    clip_idx = colmap.get("Clipping", colmap.get("Clipping Adjustment?", -1))
                    if clip_idx != -1:
                        clip_val = values[clip_idx]
                        if clip_val.strip().upper() in ("Y", "YES"):
                            clipping_val = "Yes"
                        elif clip_val.strip().upper() in ("N", "NO"):
                            clipping_val = "No"
                        else:
                            clipping_val = clip_val
        except Exception:
            pass

        #set values in table
        self.table.setItem(row, 2, QTableWidgetItem(loudness_val))
        self.table.setItem(row, 3, QTableWidgetItem(replaygain_val))
        self.table.setItem(row, 4, QTableWidgetItem(clipping_val))

    #check if file is already listed in the table/list
    def is_already_listed(self, filepath):
        for row in range(self.table.rowCount()):
            if self.table.item(row, 0).text() == filepath:
                return True
        return False

    #remove selected files from the table/list
    def remove_files(self):
        selected_rows = sorted({item.row() for item in self.table.selectedItems()}, reverse=True)
        for row in selected_rows:
            self.table.removeRow(row)

    #disable/enable all ui elements except the progress bar
    def set_ui_enabled(self, enabled: bool):
        self.add_files_btn.setEnabled(enabled)
        self.add_folder_btn.setEnabled(enabled)
        self.remove_files_btn.setEnabled(enabled)
        self.gain_btn.setEnabled(enabled)
        self.replaygain_btn.setEnabled(enabled)
        self.replaygain_input.setEnabled(enabled)
        self.table.setEnabled(enabled)

    #update table with results from worker
    def update_table_with_worker(self, updates):
        for row, loudness_val, replaygain_val, clipping_val in updates:
            self.table.setItem(row, 2, QTableWidgetItem(loudness_val))
            self.table.setItem(row, 3, QTableWidgetItem(replaygain_val))
            self.table.setItem(row, 4, QTableWidgetItem(clipping_val))

    #set progress bar value and format
    def set_progress(self, percent):
        self.progress_bar.setValue(percent)
        self.progress_bar.setFormat(f"{percent}%")

    #analyze and tag files (ReplayGain)
    def analyze_and_tag(self):
        files = []
        for row in range(self.table.rowCount()):
            files.append(self.table.item(row, 0).text())
        if not files:
            QMessageBox.information(self, "No Files", "No files to analyze.")
            return

        #show the overwrite ReplayGain tags warning
        reply = QMessageBox.question(
            self,
            "ReplayGain Tagging",
            "Some files may already have ReplayGain tags. Overwrite existing tags?",
            QMessageBox.Yes | QMessageBox.No,
            QMessageBox.Yes
        )
        if reply != QMessageBox.Yes:
            #do not perform the operation if user selects 'No'
            self.set_ui_enabled(True)
            self.set_progress(100)
            return

        #get user input for LUFS from textbox
        try:
            lufs = int(self.replaygain_input.text())
        except Exception:
            QMessageBox.warning(self, "Invalid LUFS", "Please enter a valid LUFS value.")
            return

        self.set_ui_enabled(False)
        self.set_progress(0)
        for row in range(self.table.rowCount()):
            self.table.setItem(row, 3, QTableWidgetItem("-"))
            self.table.setItem(row, 4, QTableWidgetItem("-"))

        self.worker_thread = QThread()
        self.worker = Worker(files, lufs)
        self.worker.overwrite_rg = True
        self.worker.moveToThread(self.worker_thread)
        self.worker_thread.started.connect(self.worker.run)
        self.worker.progress.connect(self.set_progress)
        self.worker.finished.connect(self._on_worker_finished_tag)
        self.worker.finished.connect(self.worker_thread.quit)
        self.worker.finished.connect(self.worker.deleteLater)
        self.worker_thread.finished.connect(self.worker_thread.deleteLater)
        self.worker_thread.start()

    #handle completion of analyze & tag worker
    def _on_worker_finished_tag(self, updates, error_logs):
        #only handle the final update (not partials)
        if not updates or not all(isinstance(u, tuple) for u in updates):
            return
        #only show popup if all rows are updated (not "-")
        all_done = all(isinstance(u, tuple) and all(x != "-" for x in u[1:]) for u in updates)
        self.update_table_with_worker(updates)
        if all_done:
            if error_logs:
                dlg = ErrorLogDialog("\n\n".join(error_logs), self)
                dlg.exec()
            QMessageBox.information(self, "Operation Complete", "Analysis and tagging have been completed.")
            self.set_ui_enabled(True)
            self.set_progress(100)
            

    def apply_gain_adjust(self):
        #gather all file paths from the table
        files = [self.table.item(row, 0).text() for row in range(self.table.rowCount())]
        if not files:
            return

        #always warn the user before applying gain
        reply = QMessageBox.question(
            self,
            "Warning: Apply Gain",
            "Applying gain to your files can irreparably damage them regardless of format. Do you want to continue?",
            QMessageBox.Yes | QMessageBox.No,
            QMessageBox.No
        )
        if reply != QMessageBox.Yes:
            return

        #get LUFS value from user input
        try:
            lufs = int(self.replaygain_input.text())
        except Exception:
            QMessageBox.warning(self, "Invalid LUFS", "Please enter a valid LUFS value.")
            return

        #disable UI and reset progress bar and table columns for operation
        self.set_ui_enabled(False)
        self.set_progress(0)
        for row in range(self.table.rowCount()):
            self.table.setItem(row, 3, QTableWidgetItem("-"))
            self.table.setItem(row, 4, QTableWidgetItem("-"))

        #start ApplyGainWorker in a background thread to keep UI responsive
        self.gain_worker_thread = QThread()
        self.gain_worker = ApplyGainWorker(files, lufs, self.table, supported_filetypes)
        self.gain_worker.moveToThread(self.gain_worker_thread)
        self.gain_worker.progress.connect(self.set_progress)
        self.gain_worker.finished.connect(self._on_apply_gain_finished)
        self.gain_worker.finished.connect(self.gain_worker_thread.quit)
        self.gain_worker.finished.connect(self.gain_worker.deleteLater)
        self.gain_worker_thread.finished.connect(self.gain_worker_thread.deleteLater)
        self.gain_worker_thread.started.connect(self.gain_worker.run)
        self.gain_worker_thread.start()

    def _on_apply_gain_finished(self, error_logs, analysis_results):
        #update the table with new analysis results after gain is applied
        for idx, loudness_val, replaygain_val, clipping_val in analysis_results:
            self.table.setItem(idx, 2, QTableWidgetItem(loudness_val))
            self.table.setItem(idx, 3, QTableWidgetItem(replaygain_val))
            self.table.setItem(idx, 4, QTableWidgetItem(clipping_val))
        #re-enable UI and set progress to 100%
        self.set_ui_enabled(True)
        self.set_progress(100)
        #show error log dialog if there were any errors
        if error_logs:
            dlg = ErrorLogDialog("\n\n".join(error_logs), self)
            dlg.exec()
        #inform the user that the operation is complete
        QMessageBox.information(self, "Operation Complete", "Gain has been applied to all files.")

#actually load and run app
if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = AudioToolGUI()
    window.show()
    sys.exit(app.exec())