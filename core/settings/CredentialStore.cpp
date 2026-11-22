#include "CredentialStore.hpp"

#include "LoggingService.hpp"

#include <fstream>
#include <sstream>

#ifndef _WIN32
#include <sys/stat.h>
#endif

namespace usn::settings {

CredentialStore::CredentialStore(std::string storePath) : storePath_(std::move(storePath)) {}

bool CredentialStore::load() {
    if (storePath_.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream in(storePath_);
    if (!in.is_open()) return false;

    creds_.clear();
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f;
        std::stringstream ss(line);
        std::string item;
        while (std::getline(ss, item, '|')) f.push_back(item);
        if (f.size() < 4) continue;
        PairingCredential c;
        c.deviceId = f[0];
        c.clientCertFingerprint = f[1];
        c.serverCertFingerprint = f[2];
        c.revoked = (f[3] == "1");
        creds_[c.deviceId] = std::move(c);
    }
    return true;
}

bool CredentialStore::save() const {
    if (storePath_.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream out(storePath_, std::ios::out | std::ios::trunc);
    if (!out.is_open()) return false;

    out << "# deviceId|clientCertFingerprint|serverCertFingerprint|revoked\n";
    for (const auto& [id, c] : creds_) {
        out << c.deviceId << "|" << c.clientCertFingerprint << "|"
            << c.serverCertFingerprint << "|" << (c.revoked ? "1" : "0") << "\n";
    }
    out.flush();
    bool ok = out.good();
    out.close();

#ifndef _WIN32
    if (ok) {
        ::chmod(storePath_.c_str(), S_IRUSR | S_IWUSR);  // 0600 owner-only
    }
#endif
    return ok;
}

bool CredentialStore::storeCredential(const PairingCredential& cred) {
    if (cred.deviceId.empty() || cred.serverCertFingerprint.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    creds_[cred.deviceId] = cred;
    LoggingService::instance().log(
        LogLevel::Info, "CredentialStore", "Stored pairing fingerprint for device id=" + cred.deviceId);
    return true;
}

bool CredentialStore::revokeCredential(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = creds_.find(deviceId);
    if (it == creds_.end()) return false;
    it->second.revoked = true;
    LoggingService::instance().log(
        LogLevel::Info, "CredentialStore", "Revoked pairing credential for device id=" + deviceId);
    return true;
}

bool CredentialStore::removeCredential(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(mutex_);
    return creds_.erase(deviceId) > 0;
}

std::optional<PairingCredential> CredentialStore::getCredential(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = creds_.find(deviceId);
    if (it == creds_.end()) return std::nullopt;
    return it->second;
}

bool CredentialStore::isTrusted(const std::string& deviceId, const std::string& serverFingerprint) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = creds_.find(deviceId);
    if (it == creds_.end() || it->second.revoked) return false;
    return it->second.serverCertFingerprint == serverFingerprint;
}

}  // namespace usn::settings
