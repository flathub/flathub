#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusUnixFileDescriptor>
#include <QObject>
#include <QSet>
#include <QStringList>

// ---------------------------------------------------------------------------
// NativeMessagingService
//
// The real service object registered on the session bus.  It owns a
// NativeMessagingAdaptor child (created in the constructor), which Qt's
// D-Bus infrastructure detects automatically when the object is registered
// via QDBusConnection::registerObject().
// ---------------------------------------------------------------------------
class NativeMessagingService : public QObject
{
    Q_OBJECT
public:
    explicit NativeMessagingService(QObject *parent = nullptr);

    // Called by the adaptor; runs pl.deno.kir.szafirhost on the host through
    // the Flatpak Development API with fdIn/fdOut/fdErr forwarded as
    // stdin/stdout/stderr.
    void spawnHost(const QStringList &args,
                   const QDBusUnixFileDescriptor &fdIn,
                   const QDBusUnixFileDescriptor &fdOut,
                   const QDBusUnixFileDescriptor &fdErr);

    int activeHostCount() const { return m_activePids.size(); }

Q_SIGNALS:
    void activeHostCountChanged(int count);

private Q_SLOTS:
    void onSpawnExited(quint32 pid, quint32 exitStatus);

private:
    QSet<quint32> m_activePids;
};

// ---------------------------------------------------------------------------
// NativeMessagingAdaptor
//
// Thin QDBusAbstractAdaptor that exposes the D-Bus interface and delegates
// the Link() call to NativeMessagingService::spawnHost().
// ---------------------------------------------------------------------------
class NativeMessagingAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "pl.deno.kir.szafirhostproxy.NativeMessaging")
public:
    explicit NativeMessagingAdaptor(NativeMessagingService *parent);

public Q_SLOTS:
    // NoReply: the browser wrapper does not wait for a return value.
    Q_NOREPLY void Link(const QStringList &args,
                        const QDBusUnixFileDescriptor &fd_in,
                        const QDBusUnixFileDescriptor &fd_out,
                        const QDBusUnixFileDescriptor &fd_err);
};
