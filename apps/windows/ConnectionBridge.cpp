#include "ConnectionBridge.hpp"

#include <QUuid>

namespace usn::ui {

ConnectionBridge::ConnectionBridge(connection::ConnectionManager& connMgr,
                                   device::DeviceManager& deviceMgr,
                                   session::SessionManager& sessionMgr,
                                   settings::SettingsManager& settingsMgr,
                                   QObject* parent)
    : QObject(parent),
      connMgr_(connMgr),
      deviceMgr_(deviceMgr),
      sessionMgr_(sessionMgr),
      settingsMgr_(settingsMgr) {}

void ConnectionBridge::setStatus(const QString& msg) {
    lastStatusMessage_ = msg;
    emit statusChanged();
}

QVariantList ConnectionBridge::devices() const {
    QVariantList out;
    for (const auto& d : deviceMgr_.listDevices()) {
        auto info = connMgr_.getConnectionInfo(d.id);
        QVariantMap m;
        m["id"] = QString::fromStdString(d.id);
        m["name"] = QString::fromStdString(d.name);
        m["type"] = QString::fromUtf8(device::toString(d.type));
        m["osVersion"] = QString::fromStdString(d.osVersion);
        m["lanAddress"] = QString::fromStdString(d.lanAddress);
        m["rustDeskPeerId"] = QString::fromStdString(d.rustDeskPeerId);
        m["state"] = QString::fromUtf8(device::toString(info.state));
        m["activeEngine"] = QString::fromUtf8(device::toString(info.activeEngine));
        m["lastOnline"] = static_cast<qint64>(d.lastOnlineEpochSec);
        m["lastError"] = QString::fromStdString(info.lastError.empty() ? d.lastError : info.lastError);
        m["switchReason"] = QString::fromStdString(info.switchReason);
        out.append(m);
    }
    return out;
}

QVariantList ConnectionBridge::history() const {
    QVariantList out;
    for (const auto& h : sessionMgr_.getHistory()) {
        QVariantMap m;
        m["sessionId"] = QString::fromStdString(h.sessionId);
        m["deviceName"] = QString::fromStdString(h.deviceName);
        m["engine"] = QString::fromUtf8(device::toString(h.engine));
        m["finalState"] = QString::fromUtf8(device::toString(h.finalState));
        m["switchReason"] = QString::fromStdString(h.switchReason);
        m["errorMessage"] = QString::fromStdString(h.errorMessage);
        out.append(m);
    }
    return out;
}

bool ConnectionBridge::darkMode() const {
    return settingsMgr_.getStreamPreferences().darkMode;
}

void ConnectionBridge::setDarkMode(bool enabled) {
    auto prefs = settingsMgr_.getStreamPreferences();
    prefs.darkMode = enabled;
    settingsMgr_.setStreamPreferences(prefs);
    settingsMgr_.save();
    emit settingsChanged();
}

void ConnectionBridge::discoverDevices() {
    auto found = connMgr_.discoverDevices(400);
    setStatus(QString("局域网发现完成，发现 %1 个端点").arg(found.size()));
    emit devicesChanged();
}

bool ConnectionBridge::addManualDevice(const QString& name,
                                       const QString& lanAddress,
                                       const QString& rustDeskId,
                                       int deviceTypeIndex) {
    device::Device d;
    d.id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    d.name = name.trimmed().toStdString();
    d.lanAddress = lanAddress.trimmed().toStdString();
    d.rustDeskPeerId = rustDeskId.trimmed().toStdString();
    d.type = (deviceTypeIndex == 1) ? device::DeviceType::AndroidDevice : device::DeviceType::WindowsPC;
    d.osVersion = (deviceTypeIndex == 1) ? "Android" : "Windows";

    bool ok = connMgr_.addDevice(d);
    if (ok) {
        deviceMgr_.save();
        setStatus(QString("已添加设备：%1（等待网络可达性探测）").arg(name));
        emit devicesChanged();
    } else {
        setStatus("添加设备失败：请检查设备名称是否有效");
    }
    return ok;
}

bool ConnectionBridge::removeDevice(const QString& deviceId) {
    bool ok = connMgr_.removeDevice(deviceId.toStdString());
    if (ok) {
        deviceMgr_.save();
        emit devicesChanged();
        emit historyChanged();
    }
    return ok;
}

bool ConnectionBridge::renameDevice(const QString& deviceId, const QString& newName) {
    bool ok = deviceMgr_.renameDevice(deviceId.toStdString(), newName.trimmed().toStdString());
    if (ok) {
        deviceMgr_.save();
        emit devicesChanged();
    }
    return ok;
}

bool ConnectionBridge::connectDevice(const QString& deviceId) {
    std::string err;
    bool ok = connMgr_.connect(deviceId.toStdString(), err);
    auto info = connMgr_.getConnectionInfo(deviceId.toStdString());
    if (ok) {
        QString msg = QString("已连接（引擎：%1）").arg(device::toString(info.activeEngine));
        if (!info.switchReason.empty()) {
            msg += QString(" — %1").arg(QString::fromStdString(info.switchReason));
        }
        setStatus(msg);
    } else {
        setStatus(QString("连接失败：%1").arg(QString::fromStdString(err)));
    }
    emit devicesChanged();
    emit historyChanged();
    return ok;
}

bool ConnectionBridge::disconnectDevice(const QString& deviceId) {
    std::string err;
    bool ok = connMgr_.disconnect(deviceId.toStdString(), err);
    setStatus(ok ? "已断开连接" : QString::fromStdString(err));
    emit devicesChanged();
    emit historyChanged();
    return ok;
}

bool ConnectionBridge::reconnectDevice(const QString& deviceId) {
    std::string err;
    bool ok = connMgr_.reconnect(deviceId.toStdString(), err);
    setStatus(ok ? "重连成功" : QString("重连失败：%1").arg(QString::fromStdString(err)));
    emit devicesChanged();
    emit historyChanged();
    return ok;
}

bool ConnectionBridge::switchEngine(const QString& deviceId, int engineIndex) {
    device::EngineType eng = device::EngineType::Auto;
    if (engineIndex == 1) eng = device::EngineType::SunshineMoonlight;
    else if (engineIndex == 2) eng = device::EngineType::RustDesk;
    std::string err;
    bool ok = connMgr_.switchEngine(deviceId.toStdString(), eng, err);
    setStatus(ok ? "已切换引擎并重新连接" : QString("切换失败：%1").arg(QString::fromStdString(err)));
    emit devicesChanged();
    emit historyChanged();
    return ok;
}

void ConnectionBridge::saveRelaySettings(int modeIndex,
                                         const QString& rendezvousHost,
                                         int rendezvousPort,
                                         const QString& relayHost,
                                         int relayPort,
                                         bool lanDirectFirst) {
    settings::RelayConfig rc;
    if (modeIndex == 1) rc.mode = settings::RelayMode::OfficialPublicPlaceholder;
    else if (modeIndex == 2) rc.mode = settings::RelayMode::SelfHostedCustom;
    else rc.mode = settings::RelayMode::Disabled;
    rc.rendezvousHost = rendezvousHost.trimmed().toStdString();
    rc.rendezvousPort = static_cast<uint16_t>(rendezvousPort);
    rc.relayHost = relayHost.trimmed().toStdString();
    rc.relayPort = static_cast<uint16_t>(relayPort);
    rc.lanDirectFirst = lanDirectFirst;
    settingsMgr_.setRelayConfig(rc);
    settingsMgr_.save();
    setStatus("中继与网络策略已保存");
    emit settingsChanged();
}

}  // namespace usn::ui
