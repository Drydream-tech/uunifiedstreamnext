#pragma once

#include "Device.hpp"

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace usn::device {

class DeviceManager {
public:
    explicit DeviceManager(std::string storagePath = "");

    bool load();
    bool save() const;

    bool addDevice(const Device& dev);
    bool removeDevice(const std::string& deviceId);
    bool renameDevice(const std::string& deviceId, const std::string& newName);
    bool updateDevice(const Device& dev);
    bool updateReachability(const std::string& deviceId, const DeviceReachability& reach);
    bool updateState(const std::string& deviceId, ConnectionState state, const std::string& error = "");
    bool markPaired(const std::string& deviceId, const std::string& certFingerprint);
    bool revokePairing(const std::string& deviceId);

    std::optional<Device> getDevice(const std::string& deviceId) const;
    std::vector<Device> listDevices() const;

private:
    std::string storagePath_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, Device> devices_;
};

}  // namespace usn::device
