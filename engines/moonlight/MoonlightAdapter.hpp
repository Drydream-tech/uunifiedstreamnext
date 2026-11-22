#pragma once

#include "../IEngineAdapter.hpp"

#include <string>

namespace usn::engines {

class MoonlightAdapter : public IEngineAdapter {
public:
    explicit MoonlightAdapter(std::string binaryPath = "");

    device::EngineType engineType() const override { return device::EngineType::SunshineMoonlight; }
    EngineCapabilities capabilities() const override;
    bool isAvailable() const override;

    bool connect(const device::Device& target,
                 const settings::StreamPreferences& prefs,
                 const settings::RelayConfig& relay,
                 EngineEventCallback onEvent,
                 std::string& outError) override;

    bool disconnect(const std::string& deviceId, std::string& outError) override;

    // Parses structured session events emitted by Moonlight IPC/CLI bridge
    static bool parseProtocolEventLine(const std::string& line, EngineEvent& outEvent);

private:
    std::string binaryPath_;
};

}  // namespace usn::engines
