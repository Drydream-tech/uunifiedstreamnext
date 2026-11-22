#include "SettingsManager.hpp"

#include "LoggingService.hpp"

#include <fstream>
#include <sstream>

namespace usn::settings {

namespace {
std::string relayModeToStr(RelayMode m) {
    switch (m) {
        case RelayMode::Disabled: return "Disabled";
        case RelayMode::OfficialPublicPlaceholder: return "OfficialPublicPlaceholder";
        case RelayMode::SelfHostedCustom: return "SelfHostedCustom";
    }
    return "Disabled";
}

RelayMode strToRelayMode(const std::string& s) {
    if (s == "OfficialPublicPlaceholder") return RelayMode::OfficialPublicPlaceholder;
    if (s == "SelfHostedCustom") return RelayMode::SelfHostedCustom;
    return RelayMode::Disabled;
}
}  // namespace

SettingsManager::SettingsManager(std::string configPath) : configPath_(std::move(configPath)) {}

bool SettingsManager::load() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream in(configPath_);
    if (!in.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);

        if (key == "relay.mode") relayConfig_.mode = strToRelayMode(val);
        else if (key == "relay.rendezvous_host") relayConfig_.rendezvousHost = val;
        else if (key == "relay.rendezvous_port") relayConfig_.rendezvousPort = static_cast<uint16_t>(std::stoi(val));
        else if (key == "relay.relay_host") relayConfig_.relayHost = val;
        else if (key == "relay.relay_port") relayConfig_.relayPort = static_cast<uint16_t>(std::stoi(val));
        else if (key == "relay.public_key") relayConfig_.publicKey = val;
        else if (key == "relay.lan_direct_first") relayConfig_.lanDirectFirst = (val == "1" || val == "true");
        else if (key == "stream.fps") streamPrefs_.targetFps = std::stoi(val);
        else if (key == "stream.bitrate_kbps") streamPrefs_.targetBitrateKbps = std::stoi(val);
        else if (key == "stream.width") streamPrefs_.width = std::stoi(val);
        else if (key == "stream.height") streamPrefs_.height = std::stoi(val);
        else if (key == "stream.hw_accel") streamPrefs_.hardwareAcceleration = (val == "1" || val == "true");
        else if (key == "ui.dark_mode") streamPrefs_.darkMode = (val == "1" || val == "true");
        else if (key == "strategy.default") streamPrefs_.defaultStrategy = val;
        else if (key.rfind("engine.path.", 0) == 0) {
            enginePaths_[key.substr(12)] = val;
        }
    }
    LoggingService::instance().log(LogLevel::Info, "SettingsManager", "Loaded settings from " + configPath_);
    return true;
}

bool SettingsManager::save() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream out(configPath_, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    out << "# UnifiedStreamNext Configuration\n";
    out << "relay.mode=" << relayModeToStr(relayConfig_.mode) << "\n";
    out << "relay.rendezvous_host=" << relayConfig_.rendezvousHost << "\n";
    out << "relay.rendezvous_port=" << relayConfig_.rendezvousPort << "\n";
    out << "relay.relay_host=" << relayConfig_.relayHost << "\n";
    out << "relay.relay_port=" << relayConfig_.relayPort << "\n";
    out << "relay.public_key=" << relayConfig_.publicKey << "\n";
    out << "relay.lan_direct_first=" << (relayConfig_.lanDirectFirst ? "true" : "false") << "\n";
    out << "stream.fps=" << streamPrefs_.targetFps << "\n";
    out << "stream.bitrate_kbps=" << streamPrefs_.targetBitrateKbps << "\n";
    out << "stream.width=" << streamPrefs_.width << "\n";
    out << "stream.height=" << streamPrefs_.height << "\n";
    out << "stream.hw_accel=" << (streamPrefs_.hardwareAcceleration ? "true" : "false") << "\n";
    out << "ui.dark_mode=" << (streamPrefs_.darkMode ? "true" : "false") << "\n";
    out << "strategy.default=" << streamPrefs_.defaultStrategy << "\n";
    for (const auto& [name, path] : enginePaths_) {
        out << "engine.path." << name << "=" << path << "\n";
    }
    out.flush();
    return out.good();
}

RelayConfig SettingsManager::getRelayConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return relayConfig_;
}

void SettingsManager::setRelayConfig(const RelayConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    relayConfig_ = config;
}

StreamPreferences SettingsManager::getStreamPreferences() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return streamPrefs_;
}

void SettingsManager::setStreamPreferences(const StreamPreferences& prefs) {
    std::lock_guard<std::mutex> lock(mutex_);
    streamPrefs_ = prefs;
}

void SettingsManager::setEngineBinaryPath(const std::string& engineName, const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    enginePaths_[engineName] = path;
}

std::string SettingsManager::getEngineBinaryPath(const std::string& engineName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = enginePaths_.find(engineName);
    return it == enginePaths_.end() ? "" : it->second;
}

}  // namespace usn::settings
