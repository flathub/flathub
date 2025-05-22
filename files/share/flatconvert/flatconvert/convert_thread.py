# flatconvert_thread.py
#
# Copyright 2024-2025 Quentin Soranzo Krebs
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: GPL-3.0-or-later

import multiprocessing
import threading
import os
import logging
from PIL import Image
from ffmpeg_progress_yield import FfmpegProgress
from gi.repository import GLib

logger = logging.getLogger(__name__)


class ConvertThread:
    '''
    TODO : change multiprocessing.pool to concurrent.futures for a best
    communication between file <-> pickled file.
    '''

    def __init__(self, progress_callback, num_workers=4):
        self._progress_callback = progress_callback
        self.task_queue = multiprocessing.JoinableQueue()
        self.result_queue = multiprocessing.Queue()
        self.num_workers = num_workers
        self.pool = None
        self.file_objects = []

    def start_conversion(self, file_objects: list):
        self.file_objects = file_objects

        # load tasks
        for file in file_objects:
            self.task_queue.put(file)

        self._file_map = {f.id: f for f in file_objects}

        # Ajouter sentinelles pour arrêter les workers
        for _ in range(self.num_workers):
            self.task_queue.put(None)

        # Create and Start pool with worker in executing _worker
        self.pool = multiprocessing.Pool(
            processes=self.num_workers,
            initializer=self._init_worker,
            initargs=(self.task_queue, self.result_queue)
        )

        # progress listener
        listener = threading.Thread(target=self._listen_progress, daemon=True)
        listener.start()

        self.pool.map(self._worker, range(self.num_workers))

        self.task_queue.join()
        self.pool.close()
        self.pool.join()


    @staticmethod
    def _init_worker(task_queue, result_queue):
        # Stocke les queues dans des variables globales pour les workers
        global _TASK_QUEUE, _RESULT_QUEUE
        _TASK_QUEUE = task_queue
        _RESULT_QUEUE = result_queue

    @staticmethod
    def _worker(_):
        '''
        TODO : Set the worker and PID attribute of the parent file
        to be able to complete the conversion.
        '''
        while True:
            file = _TASK_QUEUE.get()
            if file is None:
                _TASK_QUEUE.task_done()
                break

            try:
                mime_main = file.mime_type.split('/', 1)[0]
                if mime_main == "image":
                    logger.info("utilisation de _convert_img")
                    ConvertThread._convert_img(file)
                elif mime_main == "video":
                    logger.info("utilisation de _convert_vid")
                    ConvertThread._convert_vid(file)
                else:
                    logger.error(
                        f"format {file.mime_type} not recognized!")
                _RESULT_QUEUE.put((file.id, file.progress))
            except Exception as e:
                logger.error(f"Erreur sur {file.name}: {e}")
                _RESULT_QUEUE.put(("error", file))
            finally:
                _TASK_QUEUE.task_done()

    @staticmethod
    def _convert_img(file):
        input_path = file.input_full_path
        output_dir = file.output_dir_path
        in_fmt = file.input_format.lower()
        out_fmt = file.output_format.lower()
        base, _ = os.path.splitext(file.name)
        out_path = os.path.join(output_dir, f"{base}.{out_fmt}")

        img = Image.open(input_path)
        if in_fmt == 'png' and out_fmt in ('jpg', 'jpeg'):
            img = img.convert("RGB")
        elif out_fmt == 'gif':
            img = img.convert("P")
        img.save(out_path, out_fmt)
        file.set_progress(1)

    @staticmethod
    def _convert_vid(file):
        input_file = file.input_full_path
        output_file = file.output_full_path

        cmd = ['ffmpeg', '-y', '-i', input_file, output_file]
        ff = FfmpegProgress(cmd)
        for progress in ff.run_command_with_progress():
            logger.info("video conversion progress : %s%%", progress)
            file.set_progress(progress/100)

    def _listen_progress(self):
        total_files = len(self.file_objects)
        completed_files = 0
        overall_progress = 0

        while True:
            file_id, pct = self.result_queue.get()
            if file_id is None:
                breaks

            file = self._file_map[file_id]

            if pct == 1:  # Assuming 1 means the file is fully processed
                completed_files += 1

            # Calculate overall progress
            overall_progress = (completed_files / total_files) * 100

            GLib.idle_add(self._progress_callback, total_files, completed_files, pct, file)

    def kill_worker(self, pid) -> None:
        '''
        TODO : Command to kill the worker and recreate another one
        to maintain 5 workers.
        '''
        pass

    def terminate(self):
        self.pool.terminate()
