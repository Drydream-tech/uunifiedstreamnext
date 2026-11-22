#pragma once

#include "ConnectionStateMachine.hpp"
#include "ConnectionStrategy.hpp"
#include "../device/DeviceManager.hpp"
#include "../discovery/DiscoveryService.hpp"
#include "../network/NetworkManager.hpp"
#include "../session/SessionManager.hpp"
#include "../settings/SettingsManager.hpp"
#include "../../engines/IEngineAdapter.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace usn::connection {

struct ConnectionInfo {
    std::string deviceId;
    device::ConnectionState state{device::ConnectionState::Offline};
    device::EngineType activeEngine{device::EngineType::Auto};
    ConnectionMode mode{ConnectionMode::ModeB_RemoteDesktop};
    std::string strategyReason;
    std::string switchReason;
    std::string lastError;
    SessionMilestones milestones;
};

class ConnectionManager {
public:
    ConnectionManager(device::DeviceManager& deviceMgr,
                      discovery::DiscoveryService& discoverySvc,
                      network::NetworkManager& networkMgr,
                      session::SessionManager& sessionMgr,
                      settings::SettingsManager& settingsMgr);

    void registerAdapter(std::shared_ptr<engines::IEngineAdapter> adapter);

    // Unified public API required by specification
    std::vector<discovery::DiscoveredEndpoint> discoverDevices(int timeoutMs = 400);
    bool addDevice(const device::Device& dev);
    bool removeDevice(const std::string& deviceId);
    bool connect(const std::string& deviceId, std::string& outError);
    bool disconnect(const std::string& deviceId, std::string& outError);
    bool reconnect(const std::string& deviceId, std::string& outError);
    device::ConnectionState getConnectionState(const std::string& deviceId) const;
    ConnectionInfo getConnectionInfo(const std::string& deviceId) const;
    bool switchEngine(const std::string& deviceId, device::EngineType engine, std::string& outError);
    std::vector<engines::EngineCapabilities> getAvailableEngines() const;

    // Optional override for deterministic testing of LAN subnet detection
    void setTestInterfaceOverride(const std::vector<network::LocalInterface>* ifaces) {
        testIfaces_ = ifaces;
    }

private:
    bool attemptWithEngine(const device::Device& dev,
                           device::EngineType engine,
                           ConnectionMode mode,
                           const std::string& reason,
                           const std::string& switchReason,
                           std::string& outError);

    device::DeviceManager& deviceMgr_;
    discovery::DiscoveryService& discoverySvc_;
    network::NetworkManager& networkMgr_;
    session::SessionManager& sessionMgr_;
    settings::SettingsManager& settingsMgr_;
    ConnectionStrategy strategy_;

    mutable std::mutex mutex_;
    std::unordered_map<device::EngineType, std::shared_ptr<engines::IEngineAdapter>> adapters_;
    std::unordered_map<std::string, ConnectionStateMachine> stateMachines_;
    std::unordered_map<std::string, ConnectionInfo> connInfos_;
    std::unordered_map<std::string, std::string> activeSessionIds_;
    const std::vector<network::LocalInterface>* testIfaces_{nullptr};
};

}  // namespace usn::connection
