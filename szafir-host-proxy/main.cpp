#include <QActionGroup>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QColor>
#include <QDBusConnection>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileSystemWatcher>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPixmap>
#include <QProcess>
#include <QSettings>
#include <QVBoxLayout>

#include <KAboutData>
#include <KLocalizedString>
#include <KStatusNotifierItem>

#include "NativeHostIntegrator.h"
#include "NativeMessagingService.h"

namespace {

constexpr auto kProxyIconName = "pl.deno.kir.szafirhostproxy";
constexpr auto kSzafirAppId = "pl.deno.kir.szafir";
constexpr auto kSzafirHostAppId = "pl.deno.kir.szafirhost";

QString getProxyOrigin()
{
    QProcess process;
    process.start(
        QStringLiteral("flatpak-spawn"),
        {QStringLiteral("--host"), QStringLiteral("flatpak"), QStringLiteral("info"),
         QStringLiteral("--show-origin"), QStringLiteral("pl.deno.kir.szafirhostproxy")});
    if (!process.waitForFinished(5000)) {
        qWarning() << "Timed out while querying proxy origin";
        process.kill();
        process.waitForFinished();
        return {};
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return {};
    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

bool isFlatpakInstalled(const char *appId)
{
    QProcess process;
    process.start(
        QStringLiteral("flatpak-spawn"),
        {QStringLiteral("--host"), QStringLiteral("flatpak"), QStringLiteral("info"), QString::fromLatin1(appId)});
    if (!process.waitForFinished(5000)) {
        qWarning() << "Timed out while checking whether" << appId << "is installed";
        process.kill();
        process.waitForFinished();
        return false;
    }

    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

QIcon createWarningIcon()
{
    const QIcon baseIcon = QIcon::fromTheme(QString::fromLatin1(kProxyIconName));
    if (baseIcon.isNull())
        return baseIcon;

    QIcon warningIcon;
    const QList<int> iconSizes {16, 24, 32, 48, 64};
    for (int iconSize : iconSizes) {
        QImage image = baseIcon.pixmap(iconSize, iconSize).toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
        if (image.isNull())
            continue;

        for (int y = 0; y < image.height(); ++y) {
            QRgb *scanLine = reinterpret_cast<QRgb *>(image.scanLine(y));
            for (int x = 0; x < image.width(); ++x) {
                const QColor color = QColor::fromRgba(scanLine[x]);
                if (color.alpha() == 0)
                    continue;

                const int gray = qGray(color.red(), color.green(), color.blue());
                const int red = qMin(255, gray + 45);
                const int green = gray / 2;
                const int blue = gray / 2;
                scanLine[x] = qRgba(red, green, blue, color.alpha());
            }
        }

        warningIcon.addPixmap(QPixmap::fromImage(image));
    }

    return warningIcon.isNull() ? baseIcon : warningIcon;
}

void updateTray(
    KStatusNotifierItem *tray,
    bool hostInstalled,
    const QIcon &warningIcon,
    int activeHostCount)
{
    if (!hostInstalled) {
        tray->setStatus(KStatusNotifierItem::Active);
        if (!warningIcon.isNull())
            tray->setIconByPixmap(warningIcon);
        else
            tray->setIconByName(QString::fromLatin1(kProxyIconName));
        tray->setToolTip(
            QString::fromLatin1(kProxyIconName),
            QStringLiteral("SzafirHost Proxy"),
            i18n("SzafirHost (pl.deno.kir.szafirhost) is not installed"));
        return;
    }

    tray->setIconByName(QString::fromLatin1(kProxyIconName));
    if (activeHostCount > 0) {
        tray->setStatus(KStatusNotifierItem::Active);
        tray->setToolTip(
            QString::fromLatin1(kProxyIconName),
            QStringLiteral("SzafirHost Proxy"),
            i18n("Active hosts: %1", activeHostCount));
    } else {
        tray->setStatus(KStatusNotifierItem::Passive);
        tray->setToolTip(
            QString::fromLatin1(kProxyIconName),
            QStringLiteral("SzafirHost Proxy"),
            i18n("No active hosts"));
    }
}

// Returns the path to the user Flatpak override keyfile for appId.
// Uses HOST_XDG_DATA_HOME (set by Flatpak to the host's real XDG_DATA_HOME) so the path
// is correct even for users with a non-default data home.
static QString overrideFilePath(const QString &appId)
{
    const QString hostDataHome = qEnvironmentVariable(
        "HOST_XDG_DATA_HOME",
        QDir::homePath() + QStringLiteral("/.local/share"));
    const QString path = hostDataHome + QStringLiteral("/flatpak/overrides/") + appId;
    if (!appId.isEmpty())
        qDebug() << "Override file path for" << appId << ":" << path;
    return path;
}

// Returns the current user-overridden GDK_SCALE value for an app (empty = Auto / no override).
QString getCurrentGdkScale(const QString &appId)
{
    QSettings settings(overrideFilePath(appId), QSettings::IniFormat);
    const QString scale = settings.value(QStringLiteral("Environment/GDK_SCALE")).toString();
    qDebug() << "Read GDK_SCALE for" << appId << ":" << (scale.isEmpty() ? QStringLiteral("(Auto/unset)") : scale);
    return scale;
}

// Sets (scale = "1"/"2") or clears (scale empty) the GDK_SCALE override for an app.
void setGdkScaleOverride(const QString &appId, const QString &scale)
{
    if (scale.isEmpty())
        qDebug() << "Clearing GDK_SCALE override for" << appId;
    else
        qDebug() << "Setting GDK_SCALE=" + scale << "for" << appId;
    QSettings settings(overrideFilePath(appId), QSettings::IniFormat);
    if (scale.isEmpty())
        settings.remove(QStringLiteral("Environment/GDK_SCALE"));
    else
        settings.setValue(QStringLiteral("Environment/GDK_SCALE"), scale);
}

// Appends a submenu (Auto / 2x / 1x) to parentMenu for the given app.
// Returns an updater callable that re-checks the correct action given a new scale string.
std::function<void(const QString &)> addHiDpiMenu(QMenu *parentMenu, const QString &label, const QString &appId)
{
    auto *submenu = parentMenu->addMenu(label);
    auto *group = new QActionGroup(submenu);
    group->setExclusive(true);

    // Pairs of (action, scale value) captured by value in the returned updater.
    // Actions are parented to submenu which lives for the full app lifetime.
    struct Entry { QAction *action; QString value; };
    QList<Entry> entries;

    auto addScaleAction = [&](const QString &actionLabel, const QString &value) {
        auto *action = submenu->addAction(actionLabel);
        action->setCheckable(true);
        action->setChecked(false);
        group->addAction(action);
        entries.append({action, value});
        QObject::connect(action, &QAction::triggered, submenu, [appId, value]() {
            setGdkScaleOverride(appId, value);
        });
    };

    addScaleAction(i18n("Auto"), {});
    addScaleAction(i18n("2x scaling"), QStringLiteral("2"));
    addScaleAction(i18n("1x scaling"), QStringLiteral("1"));

    auto updater = [entries](const QString &scale) {
        for (const Entry &e : entries)
            e.action->setChecked(e.value == scale);
    };

    updater(getCurrentGdkScale(appId));

    return updater;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("szafir-host-proxy");

    KAboutData aboutData(
        QStringLiteral("szafir-host-proxy"),
        QStringLiteral("SzafirHost Proxy"),
        QStringLiteral("1.0"),
        i18n("Szafir native messaging proxy and host integrator."),
        KAboutLicense::GPL_V2,
        QStringLiteral("(C) 2026")
    );
    aboutData.addAuthor(QStringLiteral("Szafir Flatpak Maintainers"));
    aboutData.setHomepage(QStringLiteral("https://www.elektronicznypodpis.pl/"));
    KAboutData::setApplicationData(aboutData);

    app.setApplicationName(aboutData.componentName());
    app.setApplicationDisplayName(aboutData.displayName());
    app.setApplicationVersion(aboutData.version());
    app.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(aboutData.shortDescription());
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption installOpt(
        QStringLiteral("install"),
        i18n("Install native host definitions and wrappers, then exit."));
    const QCommandLineOption uninstallOpt(
        QStringLiteral("uninstall"),
        i18n("Remove native host definitions/wrappers and permissions, then exit."));
    const QCommandLineOption dryRunOpt(
        QStringLiteral("dry-run"),
        i18n("Print planned file/permission operations without performing them. Requires --install or --uninstall."));

    parser.addOption(installOpt);
    parser.addOption(uninstallOpt);
    parser.addOption(dryRunOpt);
    parser.process(app);

    const bool doInstall = parser.isSet(installOpt);
    const bool doUninstall = parser.isSet(uninstallOpt);
    const bool dryRun = parser.isSet(dryRunOpt);

    if (doInstall && doUninstall) {
        qCritical().noquote() << i18n("Use either --install or --uninstall, not both.");
        return 2;
    }

    if (dryRun && !doInstall && !doUninstall) {
        qCritical().noquote() << i18n("--dry-run requires --install or --uninstall.");
        return 2;
    }

    NativeHostIntegrator integrator(dryRun);

    if (doInstall) {
        if (!integrator.installNow()) {
            qCritical().noquote() << i18n("Failed to install native host integration.");
            return 1;
        }
        qInfo().noquote() << (dryRun ? i18n("Dry-run install complete.") : i18n("Native host integration installed."));
        return 0;
    }

    if (doUninstall) {
        if (!integrator.uninstall()) {
            qCritical().noquote() << i18n("Failed to uninstall native host integration.");
            return 1;
        }
        qInfo().noquote() << (dryRun ? i18n("Dry-run uninstall complete.") : i18n("Native host integration removed."));
        return 0;
    }

    if (!integrator.installIfNeeded()) {
        qWarning().noquote() << i18n("Failed to fully install native host integration; continuing startup.");
    }

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qCritical().noquote() << i18n("Cannot connect to the D-Bus session bus.");
        return 1;
    }

    // Single-instance enforcement: if the service name is already taken,
    // another proxy is already running — exit silently.
    if (!bus.registerService(QStringLiteral("pl.deno.kir.szafirhostproxy"))) {
        qInfo().noquote() << i18n("szafir-host-proxy is already running.");
        return 0;
    }

    // Register the D-Bus object.  Qt automatically exports any
    // QDBusAbstractAdaptor children (NativeMessagingAdaptor).
    auto *service = new NativeMessagingService(&app);
    if (!bus.registerObject(
            QStringLiteral("/pl/deno/kir/SzafirHostProxy"), service)) {
        qCritical().noquote() << i18n("Failed to register D-Bus object.");
        return 1;
    }

    // System tray indicator (KStatusNotifierItem for proper Wayland/X11 tray
    // support via the StatusNotifier protocol).
    auto *tray = new KStatusNotifierItem(
        QStringLiteral("szafir-host-proxy"), &app);
    tray->setTitle(QStringLiteral("SzafirHost Proxy"));

    const bool hostInstalled = isFlatpakInstalled(kSzafirHostAppId);
    const QIcon warningIcon = createWarningIcon();
    updateTray(tray, hostInstalled, warningIcon, service->activeHostCount());

    // When SzafirHost is not installed, clicking the tray icon shows a
    // dialog telling the user how to install it.
    if (!hostInstalled) {
        const QString origin = getProxyOrigin();
        QObject::connect(tray, &KStatusNotifierItem::activateRequested,
            &app, [origin](bool, const QPoint &) {
                QDialog dialog;
                dialog.setWindowTitle(i18n("SzafirHost Not Installed"));

                auto *layout = new QVBoxLayout(&dialog);
                layout->addWidget(new QLabel(
                    i18n("SzafirHost (pl.deno.kir.szafirhost) is not installed.\n"
                         "To use Szafir browser integration, please install it."),
                    &dialog));

                if (!origin.isEmpty()) {
                    layout->addWidget(new QLabel(i18n("Run the following command:"), &dialog));

                    auto *cmdField = new QLineEdit(
                        QStringLiteral("flatpak install %1 pl.deno.kir.szafirhost").arg(origin),
                        &dialog);
                    cmdField->setReadOnly(true);
                    cmdField->selectAll();
                    layout->addWidget(cmdField);
                }

                auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
                QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
                layout->addWidget(buttons);

                dialog.exec();
            });
    }

    // HiDPI scaling override submenus in the tray context menu.
    QMenu *trayMenu = tray->contextMenu();
    trayMenu->addSeparator();
    auto szafirUpdater = addHiDpiMenu(trayMenu, i18n("Szafir scaling"), QString::fromLatin1(kSzafirAppId));
    auto hostUpdater   = addHiDpiMenu(trayMenu, i18n("SzafirHost scaling"), QString::fromLatin1(kSzafirHostAppId));

    // Watch the overrides directory for changes
    const QString overridesDir = overrideFilePath(QString()).chopped(1); // strip trailing '/'
    qDebug() << "Setting up file system watcher on" << overridesDir;
    auto *watcher = new QFileSystemWatcher(&app);
    if (!watcher->addPath(overridesDir)) {
        // Directory may not exist yet (no overrides ever set). Create it so inotify can start.
        qDebug() << "Overrides directory does not exist; creating it...";
        QDir().mkpath(overridesDir);
        watcher->addPath(overridesDir);
    }
    QObject::connect(watcher, &QFileSystemWatcher::directoryChanged,
        &app, [szafirUpdater, hostUpdater]() {
            qDebug() << "Overrides directory changed; re-reading GDK_SCALE values...";
            szafirUpdater(getCurrentGdkScale(QString::fromLatin1(kSzafirAppId)));
            hostUpdater(getCurrentGdkScale(QString::fromLatin1(kSzafirHostAppId)));
        });

    // Update tray status and tooltip when the number of active hosts changes.
    QObject::connect(service, &NativeMessagingService::activeHostCountChanged,
        tray, [tray, hostInstalled, warningIcon](int count) {
            updateTray(tray, hostInstalled, warningIcon, count);
        });

    return app.exec();
}
