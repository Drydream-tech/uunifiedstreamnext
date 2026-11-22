#pragma once

#include <cstdint>
#include <string>

namespace usn::device {

enum class DeviceType {
    WindowsPC,
    AndroidDevice,
    LinuxHost,
    Unknown
};

enum class EngineType {
    Auto,
    SunshineMoonlight,
    RustDesk,
    AndroidHost
};

enum class ConnectionState {
    Offline,
    Discovering,
    Online,
    Connecting,
    Authenticating,
    Connected,
    Reconnecting,
    Disconnecting,
    Failed
};

struct DeviceReachability {
    bool discovered{false};
    bool networkReachable{false};
    bool sunshineHostAvailable{false};
    bool rustDeskAvailable{false};
    bool androidHostAvailable{false};
    int lastRttMs{-1};
};

struct Device {
    std::string id;
    std::string name;
    DeviceType type{DeviceType::Unknown};
    std::string osVersion;
    std::string lanAddress;
    uint16_t sunshinePort{47989};
    uint16_t rustDeskPort{21118};
    std::string rustDeskPeerId;
    EngineType preferredEngine{EngineType::Auto};
    EngineType activeEngine{EngineType::Auto};
    ConnectionState state{ConnectionState::Offline};
    DeviceReachability reachability;
    bool paired{false};
    std::string pairingFingerprint;  // Certificate fingerprint, never raw password
    int64_t lastOnlineEpochSec{0};
    std::string lastError;
};

inline const char* toString(ConnectionState s) {
    switch (s) {
        case ConnectionState::Offline:        return "Offline";
        case ConnectionState::Discovering:    return "Discovering";
        case ConnectionState::Online:         return "Online";
        case ConnectionState::Connecting:     return "Connecting";
        case ConnectionState::Authenticating: return "Authenticating";
        case ConnectionState::Connected:      return "Connected";
        case ConnectionState::Reconnecting:   return "Reconnecting";
        case ConnectionState::Disconnecting:  return "Disconnecting";
        case ConnectionState::Failed:         return "Failed";
    }
    return "Offline";
}

inline const char* toString(EngineType e) {
    switch (e) {
        case EngineType::Auto:              return "Auto";
        case EngineType::SunshineMoonlight: return "Sunshine/Moonlight";
        case EngineType::RustDesk:          return "RustDesk";
        case EngineType::AndroidHost:       return "AndroidHost";
    }
    return "Auto";
}

inline const char* toString(DeviceType t) {
    switch (t) {
        case DeviceType::WindowsPC:     return "WindowsPC";
        case DeviceType::AndroidDevice: return "AndroidDevice";
        case DeviceType::LinuxHost:     return "LinuxHost";
        case DeviceType::Unknown:       return "Unknown";
    }
    return "Unknown";
}

}  // namespace usn::device
