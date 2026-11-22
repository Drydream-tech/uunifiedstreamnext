#include "DiscoveryService.hpp"

#include "../settings/LoggingService.hpp"

#include <sstream>

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace usn::discovery {

DiscoveryService::DiscoveryService(const network::NetworkManager& netMgr) : netMgr_(netMgr) {}

std::vector<uint8_t> DiscoveryService::buildDnsSdQueryPacket(const std::string& serviceType) {
    // 12-byte DNS header: ID=0, Flags=0 (standard query), QDCOUNT=1
    std::vector<uint8_t> pkt = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    std::stringstream ss(serviceType);
    std::string label;
    while (std::getline(ss, label, '.')) {
        if (label.empty() || label.size() > 63) continue;
        pkt.push_back(static_cast<uint8_t>(label.size()));
        for (char c : label) {
            pkt.push_back(static_cast<uint8_t>(c));
        }
    }
    pkt.push_back(0x00);  // Root label
    // QTYPE = PTR (0x000C), QCLASS = IN with QU unicast-response bit (0x8001)
    pkt.push_back(0x00);
    pkt.push_back(0x0C);
    pkt.push_back(0x80);
    pkt.push_back(0x01);
    return pkt;
}

std::vector<DiscoveredEndpoint> DiscoveryService::runDiscovery(int timeoutMs) {
    std::vector<DiscoveredEndpoint> results;
    auto now = std::chrono::steady_clock::now();
    if (lastDiscovery_.time_since_epoch().count() != 0) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastDiscovery_).count();
        if (elapsed < minIntervalMs_) {
            settings::LoggingService::instance().log(
                settings::LogLevel::Debug, "DiscoveryService", "Skipping mDNS burst within rate-limit window");
            return results;
        }
    }
    lastDiscovery_ = now;

#ifndef _WIN32
    int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return results;

    sockaddr_in mcast{};
    mcast.sin_family = AF_INET;
    mcast.sin_port = htons(5353);
    ::inet_pton(AF_INET, "224.0.0.251", &mcast.sin_addr);

    auto query = buildDnsSdQueryPacket("_nvstream._tcp.local");
    (void)::sendto(sock, query.data(), query.size(), 0,
                   reinterpret_cast<sockaddr*>(&mcast), sizeof(mcast));

    pollfd pfd{};
    pfd.fd = sock;
    pfd.events = POLLIN;
    if (::poll(&pfd, 1, timeoutMs) > 0 && (pfd.revents & POLLIN)) {
        uint8_t buf[1500];
        sockaddr_in sender{};
        socklen_t slen = sizeof(sender);
        ssize_t n = ::recvfrom(sock, buf, sizeof(buf), 0,
                               reinterpret_cast<sockaddr*>(&sender), &slen);
        if (n > 12) {
            char ipBuf[INET_ADDRSTRLEN]{};
            ::inet_ntop(AF_INET, &sender.sin_addr, ipBuf, sizeof(ipBuf));
            DiscoveredEndpoint ep;
            ep.serviceName = "_nvstream._tcp.local";
            ep.hostAddress = ipBuf;
            ep.port = 47989;
            ep.isSunshine = true;
            results.push_back(std::move(ep));
        }
    }
    ::close(sock);
#endif
    return results;
}

device::DeviceReachability DiscoveryService::probeEndpoint(const std::string& ip,
                                                           uint16_t sunshinePort,
                                                           uint16_t rustDeskPort,
                                                           int timeoutMs) const {
    device::DeviceReachability reach;
    reach.discovered = true;

    auto sunProbe = netMgr_.probeTcpPort(ip, sunshinePort, timeoutMs);
    auto rdProbe = netMgr_.probeTcpPort(ip, rustDeskPort, timeoutMs);

    reach.sunshineHostAvailable = sunProbe.reachable;
    reach.rustDeskAvailable = rdProbe.reachable;
    reach.networkReachable = sunProbe.reachable || rdProbe.reachable;
    if (sunProbe.reachable) {
        reach.lastRttMs = sunProbe.rttMs;
    } else if (rdProbe.reachable) {
        reach.lastRttMs = rdProbe.rttMs;
    }
    return reach;
}

}  // namespace usn::discovery
