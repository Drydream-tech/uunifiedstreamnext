#include "ConnectionManager.hpp"

#include "../settings/LoggingService.hpp"

namespace usn::connection {

ConnectionManager::ConnectionManager(device::DeviceManager& deviceMgr,
                                     discovery::DiscoveryService& discoverySvc,
                                     network::NetworkManager& networkMgr,
                                     session::SessionManager& sessionMgr,
                                     settings::SettingsManager& settingsMgr)
    : deviceMgr_(deviceMgr),
      discoverySvc_(discoverySvc),
      networkMgr_(networkMgr),
      sessionMgr_(sessionMgr),
      settingsMgr_(settingsMgr) {}

void ConnectionManager::registerAdapter(std::shared_ptr<engines::IEngineAdapter> adapter) {
    if (!adapter) return;
    std::lock_guard<std::mutex> lock(mutex_);
    adapters_[adapter->engineType()] = std::move(adapter);
}

std::vector<discovery::DiscoveredEndpoint> ConnectionManager::discoverDevices(int timeoutMs) {
    auto found = discoverySvc_.runDiscovery(timeoutMs);
    for (const auto& ep : found) {
        auto reach = discoverySvc_.probeEndpoint(ep.hostAddress, ep.port);
        // Update matching known devices if any
        for (const auto& d : deviceMgr_.listDevices()) {
            if (d.lanAddress == ep.hostAddress) {
                deviceMgr_.updateReachability(d.id, reach);
            }
        }
    }
    return found;
}

bool ConnectionManager::addDevice(const device::Device& dev) {
    return deviceMgr_.addDevice(dev);
}

bool ConnectionManager::removeDevice(const std::string& deviceId) {
    std::string err;
    (void)disconnect(deviceId, err);
    std::lock_guard<std::mutex> lock(mutex_);
    stateMachines_.erase(deviceId);
    connInfos_.erase(deviceId);
    return deviceMgr_.removeDevice(deviceId);
}

bool ConnectionManager::attemptWithEngine(const device::Device& dev,
                                          device::EngineType engine,
                                          ConnectionMode mode,
                                          const std::string& reason,
                                          const std::string& switchReason,
                                          std::string& outError) {
    std::shared_ptr<engines::IEngineAdapter> adapter;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = adapters_.find(engine);
        if (it == adapters_.end()) {
            outError = std::string("No engine adapter registered for ") + device::toString(engine);
            return false;
        }
        adapter = it->second;
    }

    auto& sm = stateMachines_[dev.id];
    sm.reset();
    if (!sm.transitionTo(device::ConnectionState::Connecting)) {
        outError = "Invalid transition to Connecting";
        return false;
    }
    deviceMgr_.updateState(dev.id, sm.state());

    std::string sid = sessionMgr_.beginSession(dev.id, dev.name, engine, switchReason);
    activeSessionIds_[dev.id] = sid;

    ConnectionInfo info;
    info.deviceId = dev.id;
    info.state = sm.state();
    info.activeEngine = engine;
    info.mode = mode;
    info.strategyReason = reason;
    info.switchReason = switchReason;
    connInfos_[dev.id] = info;

    auto onEvent = [&](const engines::EngineEvent& ev) {
        switch (ev.type) {
            case engines::EngineEventType::ProcessStarted:
                sm.onEngineProcessStarted();
                break;
            case engines::EngineEventType::NetworkConnected:
                sm.onNetworkConnected();
                break;
            case engines::EngineEventType::Authenticated:
                sm.onAuthenticated();
                break;
            case engines::EngineEventType::SessionEstablished:
                sm.onSessionEstablished();
                break;
            case engines::EngineEventType::MediaStarted:
                sm.onMediaStreamStarted();
                break;
            case engines::EngineEventType::InputReady:
                sm.onInputChannelReady();
                break;
            case engines::EngineEventType::Disconnected:
                sm.transitionTo(device::ConnectionState::Disconnecting);
                sm.transitionTo(device::ConnectionState::Offline);
                break;
            case engines::EngineEventType::Failed:
                sm.onFailure(ev.detail);
                break;
        }
        deviceMgr_.updateState(dev.id, sm.state(), sm.lastError());
        connInfos_[dev.id].state = sm.state();
        connInfos_[dev.id].milestones = sm.milestones();
        connInfos_[dev.id].lastError = sm.lastError();
    };

    bool ok = adapter->connect(dev,
                               settingsMgr_.getStreamPreferences(),
                               settingsMgr_.getRelayConfig(),
                               onEvent,
                               outError);

    if (!ok || sm.state() != device::ConnectionState::Connected) {
        if (outError.empty()) {
            outError = sm.lastError().empty()
                           ? "Engine did not reach verified Connected session milestones"
                           : sm.lastError();
        }
        sm.onFailure(outError);
        deviceMgr_.updateState(dev.id, sm.state(), outError);
        connInfos_[dev.id].state = sm.state();
        connInfos_[dev.id].lastError = outError;
        sessionMgr_.endSession(sid, device::ConnectionState::Failed, outError);
        activeSessionIds_.erase(dev.id);
        return false;
    }

    settings::LoggingService::instance().log(
        settings::LogLevel::Info,
        "ConnectionManager",
        "Connected device=" + dev.id + " via " + device::toString(engine));
    return true;
}

bool ConnectionManager::connect(const std::string& deviceId, std::string& outError) {
    auto devOpt = deviceMgr_.getDevice(deviceId);
    if (!devOpt) {
        outError = "Device not found: " + deviceId;
        return false;
    }
    device::Device dev = *devOpt;

    StrategyContext ctx;
    ctx.isSameLan = !dev.lanAddress.empty() && networkMgr_.isSameLan(dev.lanAddress, testIfaces_);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto itM = adapters_.find(device::EngineType::SunshineMoonlight);
        ctx.moonlightEngineInstalled = (itM != adapters_.end() && itM->second->isAvailable());
        auto itR = adapters_.find(device::EngineType::RustDesk);
        ctx.rustDeskEngineInstalled = (itR != adapters_.end() && itR->second->isAvailable());
    }

    auto decision = strategy_.evaluate(dev, ctx);
    std::string primaryErr;
    if (attemptWithEngine(dev, decision.primaryEngine, decision.mode, decision.reason, "", primaryErr)) {
        return true;
    }

    // Attempt configured fallbacks and record explicit switch reason for user visibility
    for (auto fb : decision.fallbackEngines) {
        std::string switchReason = std::string("Primary engine ") +
                                   device::toString(decision.primaryEngine) +
                                   " failed (" + primaryErr + "); falling back to " +
                                   device::toString(fb) + ".";
        settings::LoggingService::instance().log(settings::LogLevel::Warn, "ConnectionManager", switchReason);
        std::string fbErr;
        if (attemptWithEngine(dev, fb, ConnectionMode::ModeC_LanFallback, decision.reason, switchReason, fbErr)) {
            return true;
        }
        primaryErr = primaryErr + " | Fallback " + device::toString(fb) + " failed: " + fbErr;
    }

    outError = primaryErr;
    return false;
}

bool ConnectionManager::disconnect(const std::string& deviceId, std::string& outError) {
    auto itInfo = connInfos_.find(deviceId);
    if (itInfo == connInfos_.end()) {
        outError.clear();
        return true;
    }

    auto engine = itInfo->second.activeEngine;
    auto& sm = stateMachines_[deviceId];
    sm.transitionTo(device::ConnectionState::Disconnecting);
    deviceMgr_.updateState(deviceId, device::ConnectionState::Disconnecting);

    std::shared_ptr<engines::IEngineAdapter> adapter;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto itA = adapters_.find(engine);
        if (itA != adapters_.end()) adapter = itA->second;
    }
    if (adapter) {
        (void)adapter->disconnect(deviceId, outError);
    }

    sm.transitionTo(device::ConnectionState::Offline);
    deviceMgr_.updateState(deviceId, device::ConnectionState::Offline);
    itInfo->second.state = device::ConnectionState::Offline;

    auto itSid = activeSessionIds_.find(deviceId);
    if (itSid != activeSessionIds_.end()) {
        sessionMgr_.endSession(itSid->second, device::ConnectionState::Offline);
        activeSessionIds_.erase(itSid);
    }
    return true;
}

bool ConnectionManager::reconnect(const std::string& deviceId, std::string& outError) {
    std::string discErr;
    (void)disconnect(deviceId, discErr);
    return connect(deviceId, outError);
}

device::ConnectionState ConnectionManager::getConnectionState(const std::string& deviceId) const {
    auto it = stateMachines_.find(deviceId);
    if (it != stateMachines_.end()) {
        return it->second.state();
    }
    auto dev = deviceMgr_.getDevice(deviceId);
    return dev ? dev->state : device::ConnectionState::Offline;
}

ConnectionInfo ConnectionManager::getConnectionInfo(const std::string& deviceId) const {
    auto it = connInfos_.find(deviceId);
    if (it != connInfos_.end()) {
        return it->second;
    }
    ConnectionInfo empty;
    empty.deviceId = deviceId;
    empty.state = getConnectionState(deviceId);
    return empty;
}

bool ConnectionManager::switchEngine(const std::string& deviceId,
                                     device::EngineType engine,
                                     std::string& outError) {
    auto devOpt = deviceMgr_.getDevice(deviceId);
    if (!devOpt) {
        outError = "Device not found: " + deviceId;
        return false;
    }
    device::Device dev = *devOpt;
    dev.preferredEngine = engine;
    deviceMgr_.updateDevice(dev);
    return reconnect(deviceId, outError);
}

std::vector<engines::EngineCapabilities> ConnectionManager::getAvailableEngines() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<engines::EngineCapabilities> out;
    for (const auto& [type, adapter] : adapters_) {
        out.push_back(adapter->capabilities());
    }
    return out;
}

}  // namespace usn::connection
