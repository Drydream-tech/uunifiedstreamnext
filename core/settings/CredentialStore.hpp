#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace usn::settings {

struct PairingCredential {
    std::string deviceId;
    std::string clientCertFingerprint;
    std::string serverCertFingerprint;
    bool revoked{false};
};

// Stores pairing metadata/fingerprints with restricted OS permissions (0600 on POSIX).
// Never logs secrets or raw keys.
class CredentialStore {
public:
    explicit CredentialStore(std::string storePath);

    bool load();
    bool save() const;

    bool storeCredential(const PairingCredential& cred);
    bool revokeCredential(const std::string& deviceId);
    bool removeCredential(const std::string& deviceId);
    std::optional<PairingCredential> getCredential(const std::string& deviceId) const;
    bool isTrusted(const std::string& deviceId, const std::string& serverFingerprint) const;

private:
    std::string storePath_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, PairingCredential> creds_;
};

}  // namespace usn::settings
