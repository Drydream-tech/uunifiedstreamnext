#include "DeviceManager.hpp"

#include "../settings/LoggingService.hpp"

#include <chrono>
#include <fstream>
#include <sstream>

namespace usn::device {

namespace {
std::vector<std::string> splitPipe(const std::string& line) {
    std::vector<std::string> parts;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, '|')) {
        parts.push_back(item);
    }
    return parts;
}
}  // namespace

DeviceManager::DeviceManager(std::string storagePath) : storagePath_(std::move(storagePath)) {}

bool DeviceManager::load() {
    if (storagePath_.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream in(storagePath_);
    if (!in.is_open()) return false;

    devices_.clear();
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto f = splitPipe(line);
        if (f.size() < 9) continue;
        Device d;
        d.id = f[0];
        d.name = f[1];
        d.type = static_cast<DeviceType>(std::stoi(f[2]));
        d.osVersion = f[3];
        d.lanAddress = f[4];
        d.sunshinePort = static_cast<uint16_t>(std::stoi(f[5]));
        d.rustDeskPeerId = f[6];
        d.preferredEngine = static_cast<EngineType>(std::stoi(f[7]));
        d.paired = (f[8] == "1");
        if (f.size() > 9) d.pairingFingerprint = f[9];
        // Never assume a loaded device is online until probed
        d.state = ConnectionState::Offline;
        devices_[d.id] = std::move(d);
    }
    return true;
}

bool DeviceManager::save() const {
    if (storagePath_.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream out(storagePath_, std::ios::out | std::ios::trunc);
    if (!out.is_open()) return false;
    out << "# id|name|type|os|lanAddress|sunshinePort|rustDeskPeerId|preferredEngine|paired|fingerprint\n";
    for (const auto& [id, d] : devices_) {
        out << d.id << "|" << d.name << "|" << static_cast<int>(d.type) << "|"
            << d.osVersion << "|" << d.lanAddress << "|" << d.sunshinePort << "|"
            << d.rustDeskPeerId << "|" << static_cast<int>(d.preferredEngine) << "|"
            << (d.paired ? "1" : "0") << "|" << d.pairingFingerprint << "\n";
    }
    out.flush();
    return out.good();
}

bool DeviceManager::addDevice(const Device& dev) {
    if (dev.id.empty() || dev.name.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    if (devices_.count(dev.id)) return false;
    Device copy = dev;
    // Presence in list does not imply online status
    if (!copy.reachability.networkReachable) {
        copy.state = ConnectionState::Offline;
    }
    devices_[copy.id] = std::move(copy);
    settings::LoggingService::instance().log(
        settings::LogLevel::Info, "DeviceManager", "Added device id=" + dev.id + " name=" + dev.name);
    return true;
}

bool DeviceManager::removeDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto erased = devices_.erase(deviceId);
    if (erased > 0) {
        settings::LoggingService::instance().log(
            settings::LogLevel::Info, "DeviceManager", "Removed device id=" + deviceId);
    }
    return erased > 0;
}

bool DeviceManager::renameDevice(const std::string& deviceId, const std::string& newName) {
    if (newName.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(deviceId);
    if (it == devices_.end()) return false;
    it->second.name = newName;
    return true;
}

bool DeviceManager::updateDevice(const Device& dev) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(dev.id);
    if (it == devices_.end()) return false;
    it->second = dev;
    return true;
}

bool DeviceManager::updateReachability(const std::string& deviceId, const DeviceReachability& reach) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(deviceId);
    if (it == devices_.end()) return false;
    it->second.reachability = reach;
    if (reach.networkReachable && (reach.sunshineHostAvailable || reach.rustDeskAvailable || reach.androidHostAvailable)) {
        auto now = std::chrono::system_clock::now();
        it->second.lastOnlineEpochSec =
            std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        if (it->second.state == ConnectionState::Offline || it->second.state == ConnectionState::Discovering) {
            it->second.state = ConnectionState::Online;
        }
    } else if (!reach.networkReachable && it->second.state == ConnectionState::Online) {
        it->second.state = ConnectionState::Offline;
    }
    return true;
}

bool DeviceManager::updateState(const std::string& deviceId, ConnectionState state, const std::string& error) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(deviceId);
    if (it == devices_.end()) return false;
    it->second.state = state;
    if (!error.empty() || state == ConnectionState::Connected || state == ConnectionState::Online) {
        it->second.lastError = error;
    }
    return true;
}

bool DeviceManager::markPaired(const std::string& deviceId, const std::string& certFingerprint) {
    if (certFingerprint.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(deviceId);
    if (it == devices_.end()) return false;
    it->second.paired = true;
    it->second.pairingFingerprint = certFingerprint;
    return true;
}

bool DeviceManager::revokePairing(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(deviceId);
    if (it == devices_.end()) return false;
    it->second.paired = false;
    it->second.pairingFingerprint.clear();
    settings::LoggingService::instance().log(
        settings::LogLevel::Info, "DeviceManager", "Revoked pairing credentials for device id=" + deviceId);
    return true;
}

std::optional<Device> DeviceManager::getDevice(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(deviceId);
    if (it == devices_.end()) return std::nullopt;
    return it->second;
}

std::vector<Device> DeviceManager::listDevices() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Device> out;
    out.reserve(devices_.size());
    for (const auto& [id, d] : devices_) {
        out.push_back(d);
    }
    return out;
}

}  // namespace usn::device
