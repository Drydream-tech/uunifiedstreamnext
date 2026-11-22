#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

namespace usn::settings {

enum class RelayMode {
    Disabled,
    OfficialPublicPlaceholder,  // Reserved for future official relay; not operated in P0
    SelfHostedCustom            // User-configured TURN / hbbs / hbbr
};

struct RelayConfig {
    RelayMode mode{RelayMode::Disabled};
    std::string rendezvousHost;   // e.g. hbbs host
    uint16_t rendezvousPort{21116};
    std::string relayHost;        // e.g. hbbr or TURN host
    uint16_t relayPort{21117};
    std::string publicKey;        // Non-secret server public key
    bool lanDirectFirst{true};    // Enforces LAN P2P direct streaming preference
};

struct StreamPreferences {
    int targetFps{60};
    int targetBitrateKbps{20000};
    int width{1920};
    int height{1080};
    bool hardwareAcceleration{true};
    bool darkMode{false};
    std::string defaultStrategy{"Auto"};  // Auto | SunshineMoonlight | RustDesk
};

class SettingsManager {
public:
    explicit SettingsManager(std::string configPath);

    bool load();
    bool save() const;

    RelayConfig getRelayConfig() const;
    void setRelayConfig(const RelayConfig& config);

    StreamPreferences getStreamPreferences() const;
    void setStreamPreferences(const StreamPreferences& prefs);

    void setEngineBinaryPath(const std::string& engineName, const std::string& path);
    std::string getEngineBinaryPath(const std::string& engineName) const;

private:
    std::string configPath_;
    mutable std::mutex mutex_;
    RelayConfig relayConfig_;
    StreamPreferences streamPrefs_;
    std::unordered_map<std::string, std::string> enginePaths_;
};

}  // namespace usn::settings
