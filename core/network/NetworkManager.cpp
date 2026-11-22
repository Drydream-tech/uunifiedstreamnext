#include "NetworkManager.hpp"

#include <chrono>
#include <cstring>

#ifndef _WIN32
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#endif

namespace usn::network {

bool NetworkManager::parseIPv4(const std::string& ipStr, uint32_t& outHostOrder) {
    in_addr addr{};
    if (inet_pton(AF_INET, ipStr.c_str(), &addr) != 1) {
        return false;
    }
    outHostOrder = ntohl(addr.s_addr);
    return true;
}

std::string NetworkManager::formatIPv4(uint32_t hostOrder) {
    in_addr addr{};
    addr.s_addr = htonl(hostOrder);
    char buf[INET_ADDRSTRLEN]{};
    if (!inet_ntop(AF_INET, &addr, buf, sizeof(buf))) return "";
    return std::string(buf);
}

bool NetworkManager::isPrivateRFC1918(uint32_t ip) {
    return ((ip & 0xFF000000u) == 0x0A000000u) ||
           ((ip & 0xFFF00000u) == 0xAC100000u) ||
           ((ip & 0xFFFF0000u) == 0xC0A80000u) ||
           ((ip & 0xFFFF0000u) == 0xA9FE0000u);
}

std::vector<LocalInterface> NetworkManager::enumerateLocalInterfaces() const {
    std::vector<LocalInterface> result;
#ifndef _WIN32
    ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) != 0 || !ifaddr) {
        return result;
    }
    for (ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
        if (!ifa->ifa_netmask) continue;

        auto* sin = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
        auto* mask = reinterpret_cast<sockaddr_in*>(ifa->ifa_netmask);

        LocalInterface li;
        li.name = ifa->ifa_name ? ifa->ifa_name : "";
        li.ipv4HostOrder = ntohl(sin->sin_addr.s_addr);
        li.netmaskHostOrder = ntohl(mask->sin_addr.s_addr);
        li.isLoopback = (ifa->ifa_flags & IFF_LOOPBACK) != 0;
        li.isUp = (ifa->ifa_flags & IFF_UP) != 0;
        result.push_back(std::move(li));
    }
    freeifaddrs(ifaddr);
#else
    ULONG bufLen = 15000;
    std::vector<uint8_t> buf(bufLen);
    auto* addrs = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data());
    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, addrs, &bufLen) == NO_ERROR) {
        for (auto* aa = addrs; aa != nullptr; aa = aa->Next) {
            bool isUp = (aa->OperStatus == IfOperStatusUp);
            bool isLoopback = (aa->IfType == IF_TYPE_SOFTWARE_LOOPBACK);
            for (auto* ua = aa->FirstUnicastAddress; ua != nullptr; ua = ua->Next) {
                if (ua->Address.lpSockaddr->sa_family != AF_INET) continue;
                auto* sin = reinterpret_cast<sockaddr_in*>(ua->Address.lpSockaddr);
                uint32_t mask = (ua->OnLinkPrefixLength == 0) ? 0 : (0xFFFFFFFFu << (32 - ua->OnLinkPrefixLength));
                LocalInterface li;
                li.name = aa->AdapterName ? aa->AdapterName : "";
                li.ipv4HostOrder = ntohl(sin->sin_addr.s_addr);
                li.netmaskHostOrder = mask;
                li.isLoopback = isLoopback;
                li.isUp = isUp;
                result.push_back(std::move(li));
            }
        }
    }
#endif
    return result;
}

bool NetworkManager::isSameLan(const std::string& targetIp, const std::vector<LocalInterface>* overrideIfaces) const {
    uint32_t target = 0;
    if (!parseIPv4(targetIp, target)) {
        return false;
    }

    std::vector<LocalInterface> ownedIfaces;
    const std::vector<LocalInterface>& ifaces =
        overrideIfaces ? *overrideIfaces : (ownedIfaces = enumerateLocalInterfaces());

    for (const auto& iface : ifaces) {
        if (!iface.isUp || iface.netmaskHostOrder == 0) continue;
        if ((target & iface.netmaskHostOrder) == (iface.ipv4HostOrder & iface.netmaskHostOrder)) {
            return true;
        }
    }
    return false;
}

ProbeResult NetworkManager::probeTcpPort(const std::string& host, uint16_t port, int timeoutMs) const {
    ProbeResult res;
    uint32_t ipHost = 0;
    if (!parseIPv4(host, ipHost)) {
        res.error = "Invalid IPv4 address: " + host;
        return res;
    }

#ifndef _WIN32
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        res.error = "socket() failed";
        return res;
    }

    int flags = ::fcntl(sock, F_GETFL, 0);
    ::fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(ipHost);

    auto start = std::chrono::steady_clock::now();
    int rc = ::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (rc == 0) {
        res.reachable = true;
        res.rttMs = 0;
        ::close(sock);
        return res;
    }

    if (errno != EINPROGRESS) {
        res.error = std::strerror(errno);
        ::close(sock);
        return res;
    }

    pollfd pfd{};
    pfd.fd = sock;
    pfd.events = POLLOUT;
    int prc = ::poll(&pfd, 1, timeoutMs);
    if (prc > 0 && (pfd.revents & POLLOUT)) {
        int soErr = 0;
        socklen_t len = sizeof(soErr);
        if (::getsockopt(sock, SOL_SOCKET, SO_ERROR, &soErr, &len) == 0 && soErr == 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - start)
                               .count();
            res.reachable = true;
            res.rttMs = static_cast<int>(elapsed);
        } else {
            res.error = soErr ? std::strerror(soErr) : "Connection refused";
        }
    } else if (prc == 0) {
        res.error = "TCP probe timed out";
    } else {
        res.error = std::strerror(errno);
    }
    ::close(sock);
#else
    SOCKET sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        res.error = "Winsock socket() failed";
        return res;
    }
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(ipHost);

    auto start = std::chrono::steady_clock::now();
    int rc = ::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (rc == 0) {
        res.reachable = true;
        res.rttMs = 0;
        closesocket(sock);
        return res;
    }
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(sock, &writeSet);
    timeval tv{timeoutMs / 1000, (timeoutMs % 1000) * 1000};
    if (select(0, nullptr, &writeSet, nullptr, &tv) > 0) {
        int soErr = 0;
        int len = sizeof(soErr);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&soErr), &len) == 0 && soErr == 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - start)
                               .count();
            res.reachable = true;
            res.rttMs = static_cast<int>(elapsed);
        } else {
            res.error = "Winsock connection refused";
        }
    } else {
        res.error = "Winsock TCP probe timed out";
    }
    closesocket(sock);
#endif
    return res;
}

}  // namespace usn::network
