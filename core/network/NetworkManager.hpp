#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace usn::network {

struct LocalInterface {
    std::string name;
    uint32_t ipv4HostOrder{0};
    uint32_t netmaskHostOrder{0};
    bool isLoopback{false};
    bool isUp{false};
};

struct ProbeResult {
    bool reachable{false};
    int rttMs{-1};
    std::string error;
};

class NetworkManager {
public:
    NetworkManager() = default;

    // Enumerates active IPv4 interfaces from OS (getifaddrs on POSIX / GetAdaptersAddresses on Windows)
    std::vector<LocalInterface> enumerateLocalInterfaces() const;

    // Uses real interface subnet mask comparison (not naive IP string prefix matching)
    bool isSameLan(const std::string& targetIp, const std::vector<LocalInterface>* overrideIfaces = nullptr) const;

    // Non-blocking TCP connect reachability probe with bounded timeout
    ProbeResult probeTcpPort(const std::string& host, uint16_t port, int timeoutMs = 500) const;

    static bool parseIPv4(const std::string& ipStr, uint32_t& outHostOrder);
    static std::string formatIPv4(uint32_t hostOrder);
    static bool isPrivateRFC1918(uint32_t ipHostOrder);
};

}  // namespace usn::network
