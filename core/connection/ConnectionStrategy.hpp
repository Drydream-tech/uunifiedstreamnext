#pragma once

#include "../device/Device.hpp"
#include "../network/NetworkManager.hpp"

#include <string>
#include <vector>

namespace usn::connection {

enum class ConnectionMode {
    ModeA_LanStreaming,      // Same LAN + Sunshine available -> Sunshine/Moonlight
    ModeB_RemoteDesktop,     // Remote network -> RustDesk
    ModeC_LanFallback,       // Same LAN + Sunshine unavailable -> RustDesk
    ManualOverride           // Explicitly selected by user
};

struct StrategyContext {
    bool isSameLan{false};
    bool sunshineEngineInstalled{false};
    bool moonlightEngineInstalled{false};
    bool rustDeskEngineInstalled{false};
};

struct StrategyDecision {
    ConnectionMode mode{ConnectionMode::ModeB_RemoteDesktop};
    device::EngineType primaryEngine{device::EngineType::RustDesk};
    std::vector<device::EngineType> fallbackEngines;
    std::string reason;
};

class ConnectionStrategy {
public:
    StrategyDecision evaluate(const device::Device& device, const StrategyContext& ctx) const;
};

}  // namespace usn::connection
