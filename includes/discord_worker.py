from PySide6.QtCore import QTimer, QRunnable, Slot, Signal, QObject, QThreadPool
from discord_integration import get_messages


class WorkerSignals(QObject):
    update = Signal(list)


class Worker(QRunnable):
    def __init__(self, channel, *args, **kwargs):
        super(Worker, self).__init__()
        self.channel = channel
        self.args = args
        self.kwargs = kwargs
        self.signals = WorkerSignals()

    @Slot()  # QtCore.Slot
    def run(self):
        self.signals.update.emit(get_messages(self.channel))
