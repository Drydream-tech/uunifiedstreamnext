#include "../core/connection/ConnectionManager.hpp"
#include "../core/connection/ConnectionStateMachine.hpp"
#include "../core/connection/ConnectionStrategy.hpp"
#include "../core/device/DeviceManager.hpp"
#include "../core/discovery/DiscoveryService.hpp"
#include "../core/network/NetworkManager.hpp"
#include "../core/session/SessionManager.hpp"
#include "../core/settings/CredentialStore.hpp"
#include "../core/settings/LoggingService.hpp"
#include "../core/settings/SettingsManager.hpp"
#include "../engines/common/SubprocessRunner.hpp"
#include "../engines/moonlight/MoonlightAdapter.hpp"
#include "../engines/rustdesk/RustDeskAdapter.hpp"
#include "../engines/sunshine/SunshineHostManager.hpp"

#include <arpa/inet.h>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace usn;

namespace {

int gPassed = 0;
#define RUN_TEST(fn)                                                    \
    do {                                                                \
        fn();                                                           \
        ++gPassed;                                                      \
        std::cout << "[PASS] " << #fn << std::endl;                     \
    } while (0)

class TestHarnessAdapter : public engines::IEngineAdapter {
public:
    TestHarnessAdapter(device::EngineType type, bool available, bool emitFullMilestones, bool failConnect)
        : type_(type), available_(available), emitFull_(emitFullMilestones), failConnect_(failConnect) {}

    device::EngineType engineType() const override { return type_; }
    bool isAvailable() const override { return available_; }
    engines::EngineCapabilities capabilities() const override {
        engines::EngineCapabilities c;
        c.type = type_;
        c.displayName = device::toString(type_);
        c.binaryAvailable = available_;
        return c;
    }

    bool connect(const device::Device& /*target*/,
                 const settings::StreamPreferences& /*prefs*/,
                 const settings::RelayConfig& /*relay*/,
                 engines::EngineEventCallback onEvent,
                 std::string& outError) override {
        if (failConnect_) {
            outError = "Simulated upstream handshake timeout";
            if (onEvent) onEvent({engines::EngineEventType::Failed, outError});
            return false;
        }
        if (onEvent) onEvent({engines::EngineEventType::ProcessStarted, "spawned"});
        if (!emitFull_) return true;
        if (onEvent) {
            onEvent({engines::EngineEventType::NetworkConnected, "tcp ok"});
            onEvent({engines::EngineEventType::Authenticated, "cert ok"});
            onEvent({engines::EngineEventType::SessionEstablished, "session ok"});
            onEvent({engines::EngineEventType::MediaStarted, "rtp video active"});
            onEvent({engines::EngineEventType::InputReady, "input channel ready"});
        }
        return true;
    }

    bool disconnect(const std::string& /*deviceId*/, std::string& outError) override {
        outError.clear();
        return true;
    }

private:
    device::EngineType type_;
    bool available_;
    bool emitFull_;
    bool failConnect_;
};

void testLoggingSecretRedaction() {
    auto& logger = settings::LoggingService::instance();
    logger.clearRecent();
    logger.setMinLevel(settings::LogLevel::Debug);
    logger.log(settings::LogLevel::Info, "Auth", "Connecting with password=SuperSecret123 pin:9988 token=abcXYZ");
    auto entries = logger.recentEntries(1);
    assert(entries.size() == 1);
    assert(entries[0].find("SuperSecret123") == std::string::npos);
    assert(entries[0].find("9988") == std::string::npos);
    assert(entries[0].find("abcXYZ") == std::string::npos);
    assert(entries[0].find("[REDACTED]") != std::string::npos);
}

void testSettingsPersistence() {
    std::string path = "/tmp/usn_test_settings.conf";
    std::filesystem::remove(path);

    settings::SettingsManager sm(path);
    settings::RelayConfig rc;
    rc.mode = settings::RelayMode::SelfHostedCustom;
    rc.rendezvousHost = "relay.example.internal";
    rc.rendezvousPort = 21116;
    rc.relayHost = "relay.example.internal";
    rc.relayPort = 21117;
    rc.lanDirectFirst = true;
    sm.setRelayConfig(rc);

    settings::StreamPreferences sp;
    sp.targetFps = 120;
    sp.targetBitrateKbps = 45000;
    sp.darkMode = true;
    sm.setStreamPreferences(sp);
    assert(sm.save());

    settings::SettingsManager sm2(path);
    assert(sm2.load());
    assert(sm2.getRelayConfig().mode == settings::RelayMode::SelfHostedCustom);
    assert(sm2.getRelayConfig().rendezvousHost == "relay.example.internal");
    assert(sm2.getStreamPreferences().targetFps == 120);
    assert(sm2.getStreamPreferences().darkMode == true);
    std::filesystem::remove(path);
}

void testCredentialStorePermissionsAndRevocation() {
    std::string path = "/tmp/usn_test_creds.db";
    std::filesystem::remove(path);

    settings::CredentialStore cs(path);
    assert(cs.storeCredential({"win-01", "SHA256:CLIENT01", "SHA256:SERVER01", false}));
    assert(cs.isTrusted("win-01", "SHA256:SERVER01"));
    assert(!cs.isTrusted("win-01", "SHA256:WRONG"));
    assert(cs.save());

    struct stat st{};
    assert(::stat(path.c_str(), &st) == 0);
    assert((st.st_mode & 0777) == 0600);

    settings::CredentialStore cs2(path);
    assert(cs2.load());
    assert(cs2.isTrusted("win-01", "SHA256:SERVER01"));
    assert(cs2.revokeCredential("win-01"));
    assert(!cs2.isTrusted("win-01", "SHA256:SERVER01"));
    std::filesystem::remove(path);
}

void testDeviceManagerCrudAndReachability() {
    std::string path = "/tmp/usn_test_devices.db";
    std::filesystem::remove(path);

    device::DeviceManager dm(path);
    device::Device d;
    d.id = "win-01";
    d.name = "Office Desktop";
    d.type = device::DeviceType::WindowsPC;
    d.osVersion = "Windows 11";
    d.lanAddress = "192.168.10.25";
    d.rustDeskPeerId = "123456789";

    assert(dm.addDevice(d));
    auto loaded = dm.getDevice("win-01");
    assert(loaded.has_value());
    assert(loaded->state == device::ConnectionState::Offline);

    assert(dm.renameDevice("win-01", "Workstation Main"));
    assert(dm.markPaired("win-01", "SHA256:AABBCC"));
    assert(dm.save());

    device::DeviceManager dm2(path);
    assert(dm2.load());
    auto loaded2 = dm2.getDevice("win-01");
    assert(loaded2.has_value());
    assert(loaded2->name == "Workstation Main");
    assert(loaded2->paired == true);
    assert(loaded2->state == device::ConnectionState::Offline);

    assert(dm2.revokePairing("win-01"));
    assert(dm2.getDevice("win-01")->paired == false);
    assert(dm2.removeDevice("win-01"));
    assert(!dm2.getDevice("win-01").has_value());
    std::filesystem::remove(path);
}

void testNetworkSubnetMaskAndRealTcpProbe() {
    network::NetworkManager nm;
    uint32_t localIp = 0, mask24 = 0;
    assert(network::NetworkManager::parseIPv4("192.168.10.5", localIp));
    assert(network::NetworkManager::parseIPv4("255.255.255.0", mask24));

    std::vector<network::LocalInterface> ifaces = {{"eth0", localIp, mask24, false, true}};
    assert(nm.isSameLan("192.168.10.99", &ifaces) == true);
    assert(nm.isSameLan("192.168.20.99", &ifaces) == false);
    assert(nm.isSameLan("8.8.8.8", &ifaces) == false);

    int srv = ::socket(AF_INET, SOCK_STREAM, 0);
    assert(srv >= 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    assert(::bind(srv, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    assert(::listen(srv, 1) == 0);
    socklen_t len = sizeof(addr);
    assert(::getsockname(srv, reinterpret_cast<sockaddr*>(&addr), &len) == 0);
    uint16_t port = ntohs(addr.sin_port);

    auto probeOk = nm.probeTcpPort("127.0.0.1", port, 300);
    assert(probeOk.reachable == true);
    assert(probeOk.rttMs >= 0);
    ::close(srv);

    auto probeClosed = nm.probeTcpPort("127.0.0.1", port, 200);
    assert(probeClosed.reachable == false);
}

void testDiscoveryPacketAndRateLimit() {
    network::NetworkManager nm;
    discovery::DiscoveryService ds(nm);
    auto pkt = discovery::DiscoveryService::buildDnsSdQueryPacket("_nvstream._tcp.local");
    assert(pkt.size() > 12);
    assert(pkt[5] == 0x01);

    ds.setMinDiscoveryIntervalMs(5000);
    (void)ds.runDiscovery(10);
    auto second = ds.runDiscovery(10);
    assert(second.empty());
}

void testStateMachineRejectsFakeConnectedState() {
    connection::ConnectionStateMachine sm;
    assert(sm.state() == device::ConnectionState::Offline);
    assert(sm.transitionTo(device::ConnectionState::Connecting));

    sm.onEngineProcessStarted();
    assert(sm.transitionTo(device::ConnectionState::Connected) == false);
    assert(sm.state() == device::ConnectionState::Connecting);

    sm.onNetworkConnected();
    assert(sm.state() == device::ConnectionState::Authenticating);
    sm.onAuthenticated();
    sm.onSessionEstablished();
    sm.onMediaStreamStarted();
    assert(sm.state() == device::ConnectionState::Connected);
}

void testConnectionStrategyModes() {
    connection::ConnectionStrategy strategy;
    device::Device winDev;
    winDev.id = "pc1";
    winDev.name = "Gaming PC";
    winDev.type = device::DeviceType::WindowsPC;
    winDev.lanAddress = "192.168.1.50";
    winDev.rustDeskPeerId = "999888777";
    winDev.reachability.sunshineHostAvailable = true;
    winDev.reachability.rustDeskAvailable = true;

    connection::StrategyContext ctxA{true, true, true, true};
    auto decA = strategy.evaluate(winDev, ctxA);
    assert(decA.mode == connection::ConnectionMode::ModeA_LanStreaming);
    assert(decA.primaryEngine == device::EngineType::SunshineMoonlight);
    assert(!decA.fallbackEngines.empty() && decA.fallbackEngines[0] == device::EngineType::RustDesk);

    winDev.reachability.sunshineHostAvailable = false;
    auto decC = strategy.evaluate(winDev, ctxA);
    assert(decC.mode == connection::ConnectionMode::ModeC_LanFallback);
    assert(decC.primaryEngine == device::EngineType::RustDesk);

    connection::StrategyContext ctxB{false, true, true, true};
    auto decB = strategy.evaluate(winDev, ctxB);
    assert(decB.mode == connection::ConnectionMode::ModeB_RemoteDesktop);
    assert(decB.primaryEngine == device::EngineType::RustDesk);

    device::Device androidDev;
    androidDev.id = "phone1";
    androidDev.name = "Pixel";
    androidDev.type = device::DeviceType::AndroidDevice;
    androidDev.reachability.androidHostAvailable = true;
    auto decAnd = strategy.evaluate(androidDev, ctxA);
    assert(decAnd.primaryEngine == device::EngineType::AndroidHost);
}

void testConnectionManagerFallbackAndHonestAdapterFailure() {
    device::DeviceManager dm;
    network::NetworkManager nm;
    discovery::DiscoveryService ds(nm);
    session::SessionManager sess;
    settings::SettingsManager cfg("/tmp/usn_unused.conf");

    connection::ConnectionManager cm(dm, ds, nm, sess, cfg);
    uint32_t ip = 0, mask = 0;
    network::NetworkManager::parseIPv4("192.168.10.2", ip);
    network::NetworkManager::parseIPv4("255.255.255.0", mask);
    std::vector<network::LocalInterface> ifaces = {{"eth0", ip, mask, false, true}};
    cm.setTestInterfaceOverride(&ifaces);

    cm.registerAdapter(std::make_shared<TestHarnessAdapter>(
        device::EngineType::SunshineMoonlight, true, true, true));
    cm.registerAdapter(std::make_shared<TestHarnessAdapter>(
        device::EngineType::RustDesk, true, true, false));

    device::Device d;
    d.id = "dev-fb";
    d.name = "LAN Host";
    d.type = device::DeviceType::WindowsPC;
    d.lanAddress = "192.168.10.50";
    d.rustDeskPeerId = "555111222";
    d.reachability.sunshineHostAvailable = true;
    d.reachability.rustDeskAvailable = true;
    assert(cm.addDevice(d));

    std::string err;
    assert(cm.connect("dev-fb", err) == true);
    auto info = cm.getConnectionInfo("dev-fb");
    assert(info.state == device::ConnectionState::Connected);
    assert(info.activeEngine == device::EngineType::RustDesk);
    assert(!info.switchReason.empty());
    assert(sess.getHistory().size() == 2);

    assert(cm.disconnect("dev-fb", err) == true);
    assert(cm.getConnectionState("dev-fb") == device::ConnectionState::Offline);

    auto realMoon = std::make_shared<engines::MoonlightAdapter>("/nonexistent/moonlight");
    auto realRd = std::make_shared<engines::RustDeskAdapter>("/nonexistent/rustdesk");
    assert(!realMoon->isAvailable());
    assert(!realRd->isAvailable());
    cm.registerAdapter(realMoon);
    cm.registerAdapter(realRd);
    assert(cm.connect("dev-fb", err) == false);
    assert(cm.getConnectionState("dev-fb") == device::ConnectionState::Failed);
}

void testProtocolEventParsers() {
    engines::EngineEvent ev;
    assert(engines::MoonlightAdapter::parseProtocolEventLine("EVENT:MEDIA_STARTED", ev));
    assert(ev.type == engines::EngineEventType::MediaStarted);
    assert(engines::RustDeskAdapter::parseProtocolEventLine("RD_EVENT:AUTH_OK", ev));
    assert(ev.type == engines::EngineEventType::Authenticated);
}

void testRealSubprocessIpcEngineBridge() {
    std::string mockBin = "/tmp/usn_test_engine.sh";
    {
        std::ofstream out(mockBin);
        out << "#!/bin/sh\n"
            << "echo 'EVENT:NETWORK_CONNECTED'\n"
            << "echo 'EVENT:AUTHENTICATED'\n"
            << "echo 'EVENT:SESSION_ESTABLISHED'\n"
            << "echo 'EVENT:MEDIA_STARTED'\n"
            << "echo 'EVENT:INPUT_READY'\n"
            << "exit 0\n";
    }
    ::chmod(mockBin.c_str(), 0755);

    device::DeviceManager dm;
    network::NetworkManager nm;
    discovery::DiscoveryService ds(nm);
    session::SessionManager sess;
    settings::SettingsManager cfg("/tmp/usn_unused2.conf");
    connection::ConnectionManager cm(dm, ds, nm, sess, cfg);

    uint32_t ip = 0, mask = 0;
    network::NetworkManager::parseIPv4("192.168.10.2", ip);
    network::NetworkManager::parseIPv4("255.255.255.0", mask);
    std::vector<network::LocalInterface> ifaces = {{"eth0", ip, mask, false, true}};
    cm.setTestInterfaceOverride(&ifaces);

    cm.registerAdapter(std::make_shared<engines::MoonlightAdapter>(mockBin));

    device::Device d;
    d.id = "ipc-dev";
    d.name = "IPC Host";
    d.type = device::DeviceType::WindowsPC;
    d.lanAddress = "192.168.10.66";
    d.reachability.sunshineHostAvailable = true;
    assert(cm.addDevice(d));

    std::string err;
    assert(cm.connect("ipc-dev", err) == true);
    auto info = cm.getConnectionInfo("ipc-dev");
    assert(info.state == device::ConnectionState::Connected);
    assert(info.milestones.networkConnected);
    assert(info.milestones.authenticated);
    assert(info.milestones.sessionEstablished);
    assert(info.milestones.mediaActive);
    assert(info.milestones.inputReady);

    std::filesystem::remove(mockBin);
}

}  // namespace

int main() {
    RUN_TEST(testLoggingSecretRedaction);
    RUN_TEST(testSettingsPersistence);
    RUN_TEST(testCredentialStorePermissionsAndRevocation);
    RUN_TEST(testDeviceManagerCrudAndReachability);
    RUN_TEST(testNetworkSubnetMaskAndRealTcpProbe);
    RUN_TEST(testDiscoveryPacketAndRateLimit);
    RUN_TEST(testStateMachineRejectsFakeConnectedState);
    RUN_TEST(testConnectionStrategyModes);
    RUN_TEST(testConnectionManagerFallbackAndHonestAdapterFailure);
    RUN_TEST(testProtocolEventParsers);
    RUN_TEST(testRealSubprocessIpcEngineBridge);
    std::cout << "All " << gPassed << " core tests passed." << std::endl;
    return 0;
}
