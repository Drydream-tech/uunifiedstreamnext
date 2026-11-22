#pragma once

#include "../device/Device.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace usn::session {

struct ConnectionHistoryEntry {
    std::string sessionId;
    std::string deviceId;
    std::string deviceName;
    device::EngineType engine{device::EngineType::Auto};
    int64_t startedAtEpochSec{0};
    int64_t endedAtEpochSec{0};
    device::ConnectionState finalState{device::ConnectionState::Offline};
    std::string switchReason;
    std::string errorMessage;
};

struct ActiveSession {
    std::string sessionId;
    std::string deviceId;
    device::EngineType engine{device::EngineType::Auto};
    int64_t startedAtEpochSec{0};
    std::string switchReason;
    bool videoActive{false};
    bool audioActive{false};
    bool inputReady{false};
};

class SessionManager {
public:
    explicit SessionManager(std::string historyPath = "");

    std::string beginSession(const std::string& deviceId,
                             const std::string& deviceName,
                             device::EngineType engine,
                             const std::string& switchReason);
    void endSession(const std::string& sessionId,
                    device::ConnectionState finalState,
                    const std::string& errorMessage = "");

    std::optional<ActiveSession> getActiveSession(const std::string& deviceId) const;
    std::vector<ConnectionHistoryEntry> getHistory() const;

    bool saveHistory() const;
    bool loadHistory();

private:
    std::string historyPath_;
    mutable std::mutex mutex_;
    uint64_t nextSeq_{1};
    std::vector<ActiveSession> activeSessions_;
    std::vector<ConnectionHistoryEntry> history_;
};

}  // namespace usn::session
