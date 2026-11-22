#pragma once

#include "../device/Device.hpp"
#include "../network/NetworkManager.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace usn::discovery {

struct DiscoveredEndpoint {
    std::string serviceName;
    std::string hostAddress;
    uint16_t port{0};
    bool isSunshine{false};
    bool isRustDesk{false};
};

class DiscoveryService {
public:
    explicit DiscoveryService(const network::NetworkManager& netMgr);

    // Rate-limited mDNS/DNS-SD query (prevents continuous broadcast storms)
    std::vector<DiscoveredEndpoint> runDiscovery(int timeoutMs = 400);

    // Explicitly probes a manually entered IP address for Sunshine / RustDesk availability
    device::DeviceReachability probeEndpoint(const std::string& ip,
                                             uint16_t sunshinePort = 47989,
                                             uint16_t rustDeskPort = 21118,
                                             int timeoutMs = 400) const;

    // Builds an RFC 6762 DNS-SD PTR query packet for a service (e.g. "_nvstream._tcp.local")
    static std::vector<uint8_t> buildDnsSdQueryPacket(const std::string& serviceType);

    void setMinDiscoveryIntervalMs(int ms) { minIntervalMs_ = ms; }

private:
    const network::NetworkManager& netMgr_;
    std::chrono::steady_clock::time_point lastDiscovery_{};
    int minIntervalMs_{3000};
};

}  // namespace usn::discovery
