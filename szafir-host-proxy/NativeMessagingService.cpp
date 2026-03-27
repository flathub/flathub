#include "NativeMessagingService.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDebug>
#include <QProcessEnvironment>

#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

// a{uh}: Flatpak host-command fd map — uint (target fd number) → Unix fd handle
using FdMap = QMap<quint32, QDBusUnixFileDescriptor>;
Q_DECLARE_METATYPE(FdMap)

// ---- NativeMessagingService ------------------------------------------------

NativeMessagingService::NativeMessagingService(QObject *parent)
    : QObject(parent)
{
    qDBusRegisterMetaType<FdMap>();
    qDBusRegisterMetaType<QByteArrayList>();

    // Create the D-Bus adaptor as a child; Qt will export it automatically
    // when this object is registered on the bus.
    new NativeMessagingAdaptor(this);

    // Subscribe to HostCommandExited so we can log child process terminations.
    QDBusConnection::sessionBus().connect(
        QStringLiteral("org.freedesktop.Flatpak"),
        QStringLiteral("/org/freedesktop/Flatpak/Development"),
        QStringLiteral("org.freedesktop.Flatpak.Development"),
        QStringLiteral("HostCommandExited"),
        this,
        SLOT(onSpawnExited(quint32, quint32)));
}

static void logFd(const char *label, const QDBusUnixFileDescriptor &fd)
{
    int raw = fd.fileDescriptor();
    int flags = fcntl(raw, F_GETFL);
    int fdflags = fcntl(raw, F_GETFD);
    qDebug() << label << "fd:" << raw
             << "valid:" << fd.isValid()
             << "F_GETFL:" << flags
             << "F_GETFD:" << fdflags;
}

void NativeMessagingService::spawnHost(const QStringList &args,
                                        const QDBusUnixFileDescriptor &fdIn,
                                        const QDBusUnixFileDescriptor &fdOut,
                                        const QDBusUnixFileDescriptor &fdErr)
{
    qDebug() << "=== Link called ===";
    qDebug() << "args:" << args;

    logFd("fdIn",  fdIn);
    logFd("fdOut", fdOut);
    logFd("fdErr", fdErr);

    // Build the Flatpak Development HostCommand call.
    // Signature: HostCommand(ay cwd, aay argv, a{uh} fds, a{ss} envs, u flags) → u pid
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.Flatpak"),
        QStringLiteral("/org/freedesktop/Flatpak/Development"),
        QStringLiteral("org.freedesktop.Flatpak.Development"),
        QStringLiteral("HostCommand"));

    // cwd_path (ay) — null-terminated byte path on the host.
    QByteArray cwd("/\0", 2);

    // argv (aay) — null-terminated byte-array list
    QByteArrayList argv;
    for (const QByteArray &arg : {
             QByteArray("/usr/bin/flatpak"),
             QByteArray("run"),
             QByteArray("pl.deno.kir.szafirhost")
         }) {
        QByteArray value = arg;
        value.append('\0');
        argv.append(value);
    }
    for (const QString &arg : args) {
        QByteArray a = arg.toLocal8Bit();
        a.append('\0');
        argv.append(a);
    }

    qDebug() << "cwd_path:" << cwd.toHex() << "(" << cwd << ")";
    qDebug() << "argv[0]:" << argv.at(0).toHex() << "(" << argv.at(0) << ")";
    qDebug() << "argv count:" << argv.size();

    // fds (a{uh}) — map target fd numbers to the received Unix fds
    FdMap fds;
    fds[0] = fdIn;   // stdin
    fds[1] = fdOut;  // stdout
    fds[2] = fdErr;  // stderr

    qDebug() << "fds map: {0:" << fds[0].fileDescriptor()
             << ", 1:" << fds[1].fileDescriptor()
             << ", 2:" << fds[2].fileDescriptor() << "}";

    // Propagate critical environment variables so the host flatpak run
    // correctly identifies display parameters for the new sandbox.
    // However, do NOT propagate XAUTHORITY from the proxy sandbox as it
    // points to /run/flatpak/Xauthority which is an invalid path when
    // evaluated on the host system.
    QMap<QString, QString> envs;
    const QProcessEnvironment sysEnv = QProcessEnvironment::systemEnvironment();
    for (const QString &key : sysEnv.keys()) {
        if (key == "DISPLAY" || key == "WAYLAND_DISPLAY" || key == "XDG_RUNTIME_DIR" || key == "DBUS_SESSION_BUS_ADDRESS") {
            envs.insert(key, sysEnv.value(key));
        }
    }

    // flags: 2 = FLATPAK_HOST_COMMAND_FLAGS_WATCH_BUS
    //   Kill spawned processes if this proxy loses its D-Bus connection.
    const quint32 flags = 2;

    msg << cwd                                         // cwd_path  (ay)
        << QVariant::fromValue(argv)                   // argv      (aay)
        << QVariant::fromValue(fds)                    // fds       (a{uh})
        << QVariant::fromValue(envs)                   // envs      (a{ss})
        << flags;                                      // flags     (u)

    qDebug() << "Sending HostCommand call, msg signature:" << msg.signature();
    qDebug() << "msg arguments count:" << msg.arguments().size();

    auto *watcher = new QDBusPendingCallWatcher(
        QDBusConnection::sessionBus().asyncCall(msg), this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this,
        [this](QDBusPendingCallWatcher *self) {
            QDBusPendingReply<quint32> reply = *self;
            if (reply.isError()) {
                qWarning() << "Flatpak HostCommand failed:"
                           << reply.error().name()
                           << reply.error().message();
            } else {
                quint32 pid = reply.value();
                qDebug() << "SzafirHost spawned, host PID:" << pid;
                m_activePids.insert(pid);
                Q_EMIT activeHostCountChanged(m_activePids.size());
            }
            self->deleteLater();
        });
}

void NativeMessagingService::onSpawnExited(quint32 pid, quint32 exitStatus)
{
    // exit_status is the raw wait status from waitpid(2)
    if (WIFEXITED(exitStatus)) {
        qDebug() << "SzafirHost PID" << pid
                 << "exited normally, exit code:" << WEXITSTATUS(exitStatus);
    } else if (WIFSIGNALED(exitStatus)) {
        qDebug() << "SzafirHost PID" << pid
                 << "killed by signal:" << WTERMSIG(exitStatus);
    } else {
        qDebug() << "SzafirHost PID" << pid
                 << "exited, raw wait status:" << exitStatus;
    }

    if (m_activePids.remove(pid))
        Q_EMIT activeHostCountChanged(m_activePids.size());
}

// ---- NativeMessagingAdaptor ------------------------------------------------

NativeMessagingAdaptor::NativeMessagingAdaptor(NativeMessagingService *parent)
    : QDBusAbstractAdaptor(parent)
{
    setAutoRelaySignals(false);
}

void NativeMessagingAdaptor::Link(const QStringList &args,
                                   const QDBusUnixFileDescriptor &fd_in,
                                   const QDBusUnixFileDescriptor &fd_out,
                                   const QDBusUnixFileDescriptor &fd_err)
{
    static_cast<NativeMessagingService *>(parent())->spawnHost(
        args, fd_in, fd_out, fd_err);
}
