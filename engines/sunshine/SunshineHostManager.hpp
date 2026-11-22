#pragma once

#include "../IEngineAdapter.hpp"

#include <string>

namespace usn::engines {

// Manages bundled Sunshine Host service lifecycle (Windows Host; not used directly on Android)
class SunshineHostManager {
public:
    explicit SunshineHostManager(std::string binaryPath = "");

    bool isBinaryAvailable() const;
    bool startHost(std::string& outError);
    bool stopHost(std::string& outError);
    bool isRunning() const { return running_; }
    std::string binaryPath() const { return binaryPath_; }

private:
    std::string binaryPath_;
    bool running_{false};
};

}  // namespace usn::engines
