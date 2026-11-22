#include "MoonlightAdapter.hpp"

#include "../common/SubprocessRunner.hpp"
#include "../../core/settings/LoggingService.hpp"

#include <filesystem>
#include <vector>

namespace usn::engines {

MoonlightAdapter::MoonlightAdapter(std::string binaryPath)
    : binaryPath_(std::move(binaryPath)) {}

bool MoonlightAdapter::isAvailable() const {
    return !binaryPath_.empty() && std::filesystem::exists(binaryPath_);
}

EngineCapabilities MoonlightAdapter::capabilities() const {
    EngineCapabilities cap;
    cap.type = device::EngineType::SunshineMoonlight;
    cap.displayName = "Sunshine / Moonlight (LAN P2P)";
    cap.binaryAvailable = isAvailable();
    cap.supportsLanStreaming = true;
    cap.supportsRemoteRelay = false;
    cap.supportsHostMode = false;
    cap.supportsClientMode = true;
    cap.binaryPath = binaryPath_;
    return cap;
}

bool MoonlightAdapter::parseProtocolEventLine(const std::string& line, EngineEvent& outEvent) {
    if (line.find("EVENT:NETWORK_CONNECTED") != std::string::npos) {
        outEvent = {EngineEventType::NetworkConnected, "Moonlight TCP/RTSP handshake succeeded"};
        return true;
    }
    if (line.find("EVENT:AUTHENTICATED") != std::string::npos) {
        outEvent = {EngineEventType::Authenticated, "Moonlight client certificate verified"};
        return true;
    }
    if (line.find("EVENT:SESSION_ESTABLISHED") != std::string::npos) {
        outEvent = {EngineEventType::SessionEstablished, "Sunshine streaming session initialized"};
        return true;
    }
    if (line.find("EVENT:MEDIA_STARTED") != std::string::npos) {
        outEvent = {EngineEventType::MediaStarted, "Video/Audio RTP stream active"};
        return true;
    }
    if (line.find("EVENT:INPUT_READY") != std::string::npos) {
        outEvent = {EngineEventType::InputReady, "Keyboard/Mouse/Gamepad input channel ready"};
        return true;
    }
    if (line.find("EVENT:FAILED:") != std::string::npos) {
        auto pos = line.find("EVENT:FAILED:");
        outEvent = {EngineEventType::Failed, line.substr(pos + 13)};
        return true;
    }
    return false;
}

bool MoonlightAdapter::connect(const device::Device& target,
                               const settings::StreamPreferences& prefs,
                               const settings::RelayConfig& /*relay*/,
                               EngineEventCallback onEvent,
                               std::string& outError) {
    if (!isAvailable()) {
        outError = "Bundled Moonlight client binary is not available at: " +
                   (binaryPath_.empty() ? "<unset>" : binaryPath_);
        settings::LoggingService::instance().log(settings::LogLevel::Warn, "MoonlightAdapter", outError);
        if (onEvent) onEvent({EngineEventType::Failed, outError});
        return false;
    }
    if (target.lanAddress.empty()) {
        outError = "Target device has no LAN address for Sunshine/Moonlight streaming.";
        if (onEvent) onEvent({EngineEventType::Failed, outError});
        return false;
    }

    if (onEvent) onEvent({EngineEventType::ProcessStarted, "Launching Moonlight subprocess"});
    SubprocessRunner runner;
    std::vector<std::string> args = {
        "stream",
        target.lanAddress,
        "--fps", std::to_string(prefs.targetFps),
        "--bitrate", std::to_string(prefs.targetBitrateKbps)
    };

    bool reachedMedia = false;
    bool ok = runner.start(binaryPath_, args, [&](const std::string& line) {
        EngineEvent ev;
        if (parseProtocolEventLine(line, ev)) {
            if (ev.type == EngineEventType::MediaStarted || ev.type == EngineEventType::InputReady) {
                reachedMedia = true;
            }
            if (ev.type == EngineEventType::Failed) {
                outError = ev.detail;
            }
            if (onEvent) onEvent(ev);
        }
    }, outError);

    if (!ok || !reachedMedia) {
        if (outError.empty()) {
            outError = "Moonlight subprocess exited without establishing active media stream.";
        }
        return false;
    }
    return true;
}

bool MoonlightAdapter::disconnect(const std::string& /*deviceId*/, std::string& outError) {
    outError.clear();
    return true;
}

}  // namespace usn::engines
