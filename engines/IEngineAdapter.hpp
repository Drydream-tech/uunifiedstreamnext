#pragma once

#include "../core/device/Device.hpp"
#include "../core/settings/SettingsManager.hpp"

#include <functional>
#include <string>

namespace usn::engines {

enum class EngineEventType {
    ProcessStarted,
    NetworkConnected,
    Authenticated,
    SessionEstablished,
    MediaStarted,
    InputReady,
    Disconnected,
    Failed
};

struct EngineEvent {
    EngineEventType type{EngineEventType::Failed};
    std::string detail;
};

using EngineEventCallback = std::function<void(const EngineEvent&)>;

struct EngineCapabilities {
    device::EngineType type{device::EngineType::Auto};
    std::string displayName;
    bool binaryAvailable{false};
    bool supportsLanStreaming{false};
    bool supportsRemoteRelay{false};
    bool supportsHostMode{false};
    bool supportsClientMode{false};
    std::string binaryPath;
};

class IEngineAdapter {
public:
    virtual ~IEngineAdapter() = default;

    virtual device::EngineType engineType() const = 0;
    virtual EngineCapabilities capabilities() const = 0;
    virtual bool isAvailable() const = 0;

    // Initiates a real connection/session; emits milestone events via callback
    virtual bool connect(const device::Device& target,
                         const settings::StreamPreferences& prefs,
                         const settings::RelayConfig& relay,
                         EngineEventCallback onEvent,
                         std::string& outError) = 0;

    virtual bool disconnect(const std::string& deviceId, std::string& outError) = 0;
};

}  // namespace usn::engines
