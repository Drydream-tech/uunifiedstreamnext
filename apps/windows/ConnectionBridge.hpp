#pragma once

#include "../../core/connection/ConnectionManager.hpp"
#include "../../core/device/DeviceManager.hpp"
#include "../../core/session/SessionManager.hpp"
#include "../../core/settings/SettingsManager.hpp"

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace usn::ui {

// Strict UI-to-Core bridge: QML invokes only ConnectionBridge, never upstream engines directly.
class ConnectionBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(QVariantList history READ history NOTIFY historyChanged)
    Q_PROPERTY(QString lastStatusMessage READ lastStatusMessage NOTIFY statusChanged)
    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY settingsChanged)

public:
    ConnectionBridge(connection::ConnectionManager& connMgr,
                     device::DeviceManager& deviceMgr,
                     session::SessionManager& sessionMgr,
                     settings::SettingsManager& settingsMgr,
                     QObject* parent = nullptr);

    QVariantList devices() const;
    QVariantList history() const;
    QString lastStatusMessage() const { return lastStatusMessage_; }
    bool darkMode() const;
    void setDarkMode(bool enabled);

    Q_INVOKABLE void discoverDevices();
    Q_INVOKABLE bool addManualDevice(const QString& name,
                                     const QString& lanAddress,
                                     const QString& rustDeskId,
                                     int deviceTypeIndex);
    Q_INVOKABLE bool removeDevice(const QString& deviceId);
    Q_INVOKABLE bool renameDevice(const QString& deviceId, const QString& newName);
    Q_INVOKABLE bool connectDevice(const QString& deviceId);
    Q_INVOKABLE bool disconnectDevice(const QString& deviceId);
    Q_INVOKABLE bool reconnectDevice(const QString& deviceId);
    Q_INVOKABLE bool switchEngine(const QString& deviceId, int engineIndex);
    Q_INVOKABLE void saveRelaySettings(int modeIndex,
                                       const QString& rendezvousHost,
                                       int rendezvousPort,
                                       const QString& relayHost,
                                       int relayPort,
                                       bool lanDirectFirst);

signals:
    void devicesChanged();
    void historyChanged();
    void statusChanged();
    void settingsChanged();

private:
    void setStatus(const QString& msg);

    connection::ConnectionManager& connMgr_;
    device::DeviceManager& deviceMgr_;
    session::SessionManager& sessionMgr_;
    settings::SettingsManager& settingsMgr_;
    QString lastStatusMessage_;
};

}  // namespace usn::ui
