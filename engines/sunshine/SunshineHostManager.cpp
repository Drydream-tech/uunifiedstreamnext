#include "SunshineHostManager.hpp"

#include "../../core/settings/LoggingService.hpp"

#include <filesystem>

namespace usn::engines {

SunshineHostManager::SunshineHostManager(std::string binaryPath)
    : binaryPath_(std::move(binaryPath)) {}

bool SunshineHostManager::isBinaryAvailable() const {
    return !binaryPath_.empty() && std::filesystem::exists(binaryPath_);
}

bool SunshineHostManager::startHost(std::string& outError) {
    if (!isBinaryAvailable()) {
        outError = "Bundled Sunshine Host binary not found at: " +
                   (binaryPath_.empty() ? "<unset>" : binaryPath_) +
                   ". Build upstream LizardByte/Sunshine first.";
        settings::LoggingService::instance().log(settings::LogLevel::Error, "SunshineHost", outError);
        return false;
    }
    // Real subprocess / Windows service launch is implemented in platform build
    outError = "Sunshine Host execution requires Windows build with bundled Sunshine runtime.";
    return false;
}

bool SunshineHostManager::stopHost(std::string& outError) {
    if (!running_) {
        outError.clear();
        return true;
    }
    running_ = false;
    return true;
}

}  // namespace usn::engines
