#include "NativeHostIntegrator.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QDebug>

namespace {

constexpr const char *kMarkerFileName = "native-hosts-integrator-v1";
constexpr const char *kManifestName = "pl.deno.kir.szafirhost.json";
constexpr const char *kDbusService = "pl.deno.kir.szafirhostproxy";
constexpr const char *kInstalledWrapperPath = "/app/share/szafir-host-proxy/flatpak-host-wrapper";

enum class BrowserBase {
    Firefox,
    Chromium,
};

struct BrowserInfo {
    BrowserBase base;
    QString id;
    QString hostNativeMessagingHostsDir;
    QString flatpakNativeMessagingHostsDir;
};

QList<BrowserInfo> browsers()
{
    return {
        {BrowserBase::Firefox,  QStringLiteral("org.mozilla.firefox"),
            QStringLiteral("/.mozilla/native-messaging-hosts"),
            QStringLiteral("/.mozilla/native-messaging-hosts")},
        {BrowserBase::Firefox,  QStringLiteral("io.gitlab.librewolf-community"),
            QStringLiteral("/.librewolf/native-messaging-hosts"),
            QStringLiteral("/.librewolf/native-messaging-hosts")},
        {BrowserBase::Firefox,  QStringLiteral("net.waterfox.waterfox"),
            QStringLiteral("/.waterfox/native-messaging-hosts"),
            QStringLiteral("/.waterfox/native-messaging-hosts")},
        {BrowserBase::Chromium, QStringLiteral("com.google.Chrome"),
            QStringLiteral("/.config/google-chrome/NativeMessagingHosts"),
            QStringLiteral("/config/google-chrome/NativeMessagingHosts")},
        {BrowserBase::Chromium, QStringLiteral("com.google.ChromeDev"),
            QStringLiteral("/.config/google-chrome-unstable/NativeMessagingHosts"),
            QStringLiteral("/config/google-chrome-unstable/NativeMessagingHosts")},
        {BrowserBase::Chromium, QStringLiteral("org.chromium.Chromium"),
            QStringLiteral("/.config/chromium/NativeMessagingHosts"),
            QStringLiteral("/config/chromium/NativeMessagingHosts")},
        {BrowserBase::Chromium, QStringLiteral("io.github.ungoogled_software.ungoogled_chromium"),
            QStringLiteral("/.config/chromium/NativeMessagingHosts"),
            QStringLiteral("/config/chromium/NativeMessagingHosts")},
    };
}

bool runFlatpakOverride(const QStringList &args, bool dryRun)
{
    QStringList hostArgs;
    hostArgs << QStringLiteral("--host") << QStringLiteral("flatpak") << args;

    qInfo() << "perm-op:" << "flatpak-spawn" << hostArgs;
    if (dryRun)
        return true;

    QProcess p;
    p.start(QStringLiteral("flatpak-spawn"), hostArgs);
    if (!p.waitForFinished(15000)) {
        qWarning() << "flatpak-spawn timed out:" << hostArgs;
        return false;
    }

    if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        qWarning() << "flatpak-spawn failed:" << hostArgs
                   << "stderr:" << QString::fromLocal8Bit(p.readAllStandardError());
        return false;
    }

    return true;
}

bool ensureParentDir(const QString &filePath, bool dryRun)
{
    QFileInfo fi(filePath);
    QDir dir = fi.dir();
    if (dir.exists())
        return true;

    qInfo() << "file-op:" << "mkdir -p" << dir.path();
    if (dryRun)
        return true;

    return dir.mkpath(QStringLiteral("."));
}

bool writeExecutableFile(const QString &path, const QString &content, bool dryRun)
{
    if (!ensureParentDir(path, dryRun)) {
        qWarning() << "Failed to create parent directory for" << path;
        return false;
    }

    qInfo() << "file-op:" << "write" << path;
    qInfo() << "perm-op:" << "chmod 755" << path;
    if (dryRun)
        return true;

    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning() << "Failed to open for writing:" << path;
        return false;
    }

    QTextStream out(&f);
    out << content;

    if (!f.commit()) {
        qWarning() << "Failed to commit file:" << path;
        return false;
    }

    QFile::setPermissions(path,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                          QFileDevice::ReadGroup | QFileDevice::ExeGroup |
                          QFileDevice::ReadOther | QFileDevice::ExeOther);
    return true;
}

bool writeJsonFile(const QString &path, const QJsonObject &obj, bool dryRun)
{
    if (!ensureParentDir(path, dryRun)) {
        qWarning() << "Failed to create parent directory for" << path;
        return false;
    }

    qInfo() << "file-op:" << "write" << path;
    if (dryRun)
        return true;

    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Failed to open for writing:" << path;
        return false;
    }

    f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    if (!f.commit()) {
        qWarning() << "Failed to commit JSON file:" << path;
        return false;
    }
    return true;
}

QJsonObject manifestFor(const BrowserInfo &browser, const QString &wrapperPath)
{
    QJsonObject obj {
        {QStringLiteral("name"), QStringLiteral("pl.deno.kir.szafirhost")},
        {QStringLiteral("description"), QStringLiteral("Szafir Native Messaging Host")},
        {QStringLiteral("path"), wrapperPath},
        {QStringLiteral("type"), QStringLiteral("stdio")},
    };

    if (browser.base == BrowserBase::Firefox) {
        obj.insert(QStringLiteral("allowed_extensions"), QJsonArray {
            QStringLiteral("{5e118bad-a840-4256-bd31-296194533aac}")
        });
    } else {
        obj.insert(QStringLiteral("allowed_origins"), QJsonArray {
            QStringLiteral("chrome-extension://gjalhnomhafafofonpdihihjnbafkipc/"),
            QStringLiteral("chrome-extension://bikmiknjdohdfmehchjpbiemekemgndp/")
        });
    }

    return obj;
}

QString hostWrapperPath(const QString &browserId)
{
    return QDir::homePath() + QStringLiteral("/.local/share/szafirhost/wrappers/")
        + browserId + QStringLiteral("/flatpak-host-wrapper");
}

QString flatpakWrapperPath(const QString &browserId)
{
    return QDir::homePath() + QStringLiteral("/.var/app/") + browserId
        + QStringLiteral("/szafir-host-proxy/flatpak-host-wrapper");
}

QString hostManifestPath(const BrowserInfo &browser)
{
    return QDir::homePath() + browser.hostNativeMessagingHostsDir + QStringLiteral("/")
        + QString::fromLatin1(kManifestName);
}

QString flatpakManifestPath(const BrowserInfo &browser)
{
    return QDir::homePath() + QStringLiteral("/.var/app/") + browser.id + browser.flatpakNativeMessagingHostsDir
        + QStringLiteral("/") + QString::fromLatin1(kManifestName);
}

} // namespace

NativeHostIntegrator::NativeHostIntegrator(bool dryRun)
    : m_dryRun(dryRun)
{
    const QString cfgRoot = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    m_markerPath = cfgRoot + QStringLiteral("/") + QString::fromLatin1(kMarkerFileName);
}

bool NativeHostIntegrator::installIfNeeded()
{
    if (!ensureWrapperTemplateLoaded())
        return false;

    if (isInstalled()) {
        qInfo() << "Native host definitions already installed, skipping";
        return true;
    }

    if (!installAll())
        return false;

    return writeMarker();
}

bool NativeHostIntegrator::installNow()
{
    if (!ensureWrapperTemplateLoaded())
        return false;

    if (!installAll())
        return false;

    return writeMarker();
}

bool NativeHostIntegrator::uninstall()
{
    const bool ok = removeAll();
    const bool markerOk = removeMarker();
    return ok && markerOk;
}

bool NativeHostIntegrator::installAll()
{
    bool allOk = true;
    const auto browserList = browsers();

    for (const BrowserInfo &browser : browserList) {
        const QString hostWrapper = hostWrapperPath(browser.id);
        const QString flatpakWrapper = flatpakWrapperPath(browser.id);
        const QJsonObject hostObj = manifestFor(browser, hostWrapper);
        const QJsonObject flatpakObj = manifestFor(browser, flatpakWrapper);

        allOk = writeExecutableFile(hostWrapper, m_wrapperTemplate, m_dryRun) && allOk;
        allOk = writeExecutableFile(flatpakWrapper, m_wrapperTemplate, m_dryRun) && allOk;
        allOk = writeJsonFile(hostManifestPath(browser), hostObj, m_dryRun) && allOk;
        allOk = writeJsonFile(flatpakManifestPath(browser), flatpakObj, m_dryRun) && allOk;

        allOk = runFlatpakOverride({
            QStringLiteral("override"),
            QStringLiteral("--user"),
            QStringLiteral("--talk-name=") + QString::fromLatin1(kDbusService),
            browser.id,
        }, m_dryRun) && allOk;
    }

    return allOk;
}

bool NativeHostIntegrator::removeAll()
{
    bool allOk = true;
    const auto browserList = browsers();

    for (const BrowserInfo &browser : browserList) {
        const QString hostWrapper = hostWrapperPath(browser.id);
        const QString sandboxWrapper = flatpakWrapperPath(browser.id);
        qInfo() << "file-op:" << "remove" << hostWrapper;
        qInfo() << "file-op:" << "remove" << sandboxWrapper;
        if (!m_dryRun)
            QFile::remove(hostWrapper);
        if (!m_dryRun)
            QFile::remove(sandboxWrapper);

        const QString hostPath = hostManifestPath(browser);
        const QString flatpakPath = flatpakManifestPath(browser);
        qInfo() << "file-op:" << "remove" << hostPath;
        qInfo() << "file-op:" << "remove" << flatpakPath;
        if (!m_dryRun) {
            QFile::remove(hostPath);
            QFile::remove(flatpakPath);
        }

        allOk = runFlatpakOverride({
            QStringLiteral("override"),
            QStringLiteral("--user"),
            QStringLiteral("--no-talk-name=") + QString::fromLatin1(kDbusService),
            browser.id,
        }, m_dryRun) && allOk;
    }

    return allOk;
}

bool NativeHostIntegrator::isInstalled() const
{
    return QFileInfo::exists(m_markerPath);
}

bool NativeHostIntegrator::writeMarker() const
{
    qInfo() << "file-op:" << "write" << m_markerPath;
    if (m_dryRun)
        return true;

    if (!ensureParentDir(m_markerPath, m_dryRun)) {
        qWarning() << "Failed to create marker parent:" << m_markerPath;
        return false;
    }

    QSaveFile f(m_markerPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning() << "Failed to create marker:" << m_markerPath;
        return false;
    }

    f.write("v1\n");
    return f.commit();
}

bool NativeHostIntegrator::removeMarker() const
{
    if (!QFileInfo::exists(m_markerPath))
        return true;

    qInfo() << "file-op:" << "remove" << m_markerPath;
    if (m_dryRun)
        return true;

    return QFile::remove(m_markerPath);
}

bool NativeHostIntegrator::ensureWrapperTemplateLoaded()
{
    if (!m_wrapperTemplate.isEmpty())
        return true;

    qInfo() << "file-op:" << "read" << QString::fromLatin1(kInstalledWrapperPath);
    QFile f(QString::fromLatin1(kInstalledWrapperPath));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to read wrapper template:" << f.fileName();
        return false;
    }

    m_wrapperTemplate = QString::fromUtf8(f.readAll());
    return !m_wrapperTemplate.isEmpty();
}
