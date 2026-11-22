#include "RustDeskAdapter.hpp"

#include "../common/SubprocessRunner.hpp"
#include "../../core/settings/LoggingService.hpp"

#include <filesystem>
#include <vector>

namespace usn::engines {

RustDeskAdapter::RustDeskAdapter(std::string binaryPath)
    : binaryPath_(std::move(binaryPath)) {}

bool RustDeskAdapter::isAvailable() const {
    return !binaryPath_.empty() && std::filesystem::exists(binaryPath_);
}

EngineCapabilities RustDeskAdapter::capabilities() const {
    EngineCapabilities cap;
    cap.type = device::EngineType::RustDesk;
    cap.displayName = "RustDesk (Remote Desktop / NAT / Relay)";
    cap.binaryAvailable = isAvailable();
    cap.supportsLanStreaming = true;
    cap.supportsRemoteRelay = true;
    cap.supportsHostMode = true;
    cap.supportsClientMode = true;
    cap.binaryPath = binaryPath_;
    return cap;
}

bool RustDeskAdapter::parseProtocolEventLine(const std::string& line, EngineEvent& outEvent) {
    if (line.find("RD_EVENT:CONNECTED") != std::string::npos) {
        outEvent = {EngineEventType::NetworkConnected, "RustDesk peer/relay transport established"};
        return true;
    }
    if (line.find("RD_EVENT:AUTH_OK") != std::string::npos) {
        outEvent = {EngineEventType::Authenticated, "RustDesk peer authentication succeeded"};
        return true;
    }
    if (line.find("RD_EVENT:SESSION_OK") != std::string::npos) {
        outEvent = {EngineEventType::SessionEstablished, "RustDesk remote desktop session active"};
        return true;
    }
    if (line.find("RD_EVENT:FRAME_RECEIVED") != std::string::npos) {
        outEvent = {EngineEventType::MediaStarted, "First remote desktop frame decoded"};
        return true;
    }
    if (line.find("RD_EVENT:INPUT_READY") != std::string::npos) {
        outEvent = {EngineEventType::InputReady, "Remote input control ready"};
        return true;
    }
    if (line.find("RD_EVENT:ERROR:") != std::string::npos) {
        auto pos = line.find("RD_EVENT:ERROR:");
        outEvent = {EngineEventType::Failed, line.substr(pos + 15)};
        return true;
    }
    return false;
}

bool RustDeskAdapter::connect(const device::Device& target,
                              const settings::StreamPreferences& /*prefs*/,
                              const settings::RelayConfig& relay,
                              EngineEventCallback onEvent,
                              std::string& outError) {
    if (!isAvailable()) {
        outError = "Bundled RustDesk engine binary is not available at: " +
                   (binaryPath_.empty() ? "<unset>" : binaryPath_);
        settings::LoggingService::instance().log(settings::LogLevel::Warn, "RustDeskAdapter", outError);
        if (onEvent) onEvent({EngineEventType::Failed, outError});
        return false;
    }
    if (relay.mode == settings::RelayMode::OfficialPublicPlaceholder) {
        outError = "Official public relay server is not provisioned in P0; configure a self-hosted hbbs/hbbr relay or direct LAN address.";
        if (onEvent) onEvent({EngineEventType::Failed, outError});
        return false;
    }
    if (target.rustDeskPeerId.empty() && target.lanAddress.empty()) {
        outError = "Target device has neither RustDesk Peer ID nor LAN address.";
        if (onEvent) onEvent({EngineEventType::Failed, outError});
        return false;
    }

    if (onEvent) onEvent({EngineEventType::ProcessStarted, "Launching RustDesk isolated subprocess"});
    SubprocessRunner runner;
    std::string peerTarget = !target.rustDeskPeerId.empty() ? target.rustDeskPeerId : target.lanAddress;
    std::vector<std::string> args = {"--connect", peerTarget};
    if (relay.mode == settings::RelayMode::SelfHostedCustom && !relay.rendezvousHost.empty()) {
        args.push_back("--rendezvous");
        args.push_back(relay.rendezvousHost + ":" + std::to_string(relay.rendezvousPort));
    }

    bool reachedSession = false;
    bool ok = runner.start(binaryPath_, args, [&](const std::string& line) {
        EngineEvent ev;
        if (parseProtocolEventLine(line, ev)) {
            if (ev.type == EngineEventType::MediaStarted || ev.type == EngineEventType::InputReady) {
                reachedSession = true;
            }
            if (ev.type == EngineEventType::Failed) {
                outError = ev.detail;
            }
            if (onEvent) onEvent(ev);
        }
    }, outError);

    if (!ok || !reachedSession) {
        if (outError.empty()) {
            outError = "RustDesk subprocess exited without establishing active remote desktop session.";
        }
        return false;
    }
    return true;
}

bool RustDeskAdapter::disconnect(const std::string& /*deviceId*/, std::string& outError) {
    outError.clear();
    return true;
}

}  // namespace usn::engines
