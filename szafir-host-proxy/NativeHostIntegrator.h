#pragma once

#include <QString>

class NativeHostIntegrator
{
public:
    explicit NativeHostIntegrator(bool dryRun = false);

    // Installs manifests/wrappers once per marker version.
    bool installIfNeeded();

    // Installs regardless of marker state.
    bool installNow();

    // Removes only Szafir-managed artifacts and overrides.
    bool uninstall();

    bool isDryRun() const { return m_dryRun; }

private:
    bool installAll();
    bool removeAll();
    bool isInstalled() const;
    bool writeMarker() const;
    bool removeMarker() const;
    bool ensureWrapperTemplateLoaded();

    QString m_markerPath;
    QString m_wrapperTemplate;
    bool m_dryRun;
};
