#include "SessionManager.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>

namespace usn::session {

SessionManager::SessionManager(std::string historyPath) : historyPath_(std::move(historyPath)) {}

std::string SessionManager::beginSession(const std::string& deviceId,
                                         const std::string& deviceName,
                                         device::EngineType engine,
                                         const std::string& switchReason) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now().time_since_epoch())
                   .count();
    std::string sid = "sess-" + std::to_string(now) + "-" + std::to_string(nextSeq_++);

    ActiveSession active;
    active.sessionId = sid;
    active.deviceId = deviceId;
    active.engine = engine;
    active.startedAtEpochSec = now;
    active.switchReason = switchReason;
    activeSessions_.push_back(active);

    ConnectionHistoryEntry entry;
    entry.sessionId = sid;
    entry.deviceId = deviceId;
    entry.deviceName = deviceName;
    entry.engine = engine;
    entry.startedAtEpochSec = now;
    entry.finalState = device::ConnectionState::Connecting;
    entry.switchReason = switchReason;
    history_.push_back(std::move(entry));

    return sid;
}

void SessionManager::endSession(const std::string& sessionId,
                                device::ConnectionState finalState,
                                const std::string& errorMessage) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now().time_since_epoch())
                   .count();

    activeSessions_.erase(
        std::remove_if(activeSessions_.begin(), activeSessions_.end(),
                       [&](const ActiveSession& s) { return s.sessionId == sessionId; }),
        activeSessions_.end());

    for (auto& h : history_) {
        if (h.sessionId == sessionId) {
            h.endedAtEpochSec = now;
            h.finalState = finalState;
            h.errorMessage = errorMessage;
            break;
        }
    }
}

std::optional<ActiveSession> SessionManager::getActiveSession(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& s : activeSessions_) {
        if (s.deviceId == deviceId) return s;
    }
    return std::nullopt;
}

std::vector<ConnectionHistoryEntry> SessionManager::getHistory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return history_;
}

bool SessionManager::saveHistory() const {
    if (historyPath_.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream out(historyPath_, std::ios::out | std::ios::trunc);
    if (!out.is_open()) return false;
    out << "# sessionId|deviceId|deviceName|engine|startedAt|endedAt|finalState|switchReason|error\n";
    for (const auto& h : history_) {
        out << h.sessionId << "|" << h.deviceId << "|" << h.deviceName << "|"
            << static_cast<int>(h.engine) << "|" << h.startedAtEpochSec << "|"
            << h.endedAtEpochSec << "|" << static_cast<int>(h.finalState) << "|"
            << h.switchReason << "|" << h.errorMessage << "\n";
    }
    out.flush();
    return out.good();
}

bool SessionManager::loadHistory() {
    if (historyPath_.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream in(historyPath_);
    if (!in.is_open()) return false;
    history_.clear();
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string item;
        while (std::getline(ss, item, '|')) parts.push_back(item);
        if (parts.size() < 9) continue;
        ConnectionHistoryEntry e;
        e.sessionId = parts[0];
        e.deviceId = parts[1];
        e.deviceName = parts[2];
        e.engine = static_cast<device::EngineType>(std::stoi(parts[3]));
        e.startedAtEpochSec = std::stoll(parts[4]);
        e.endedAtEpochSec = std::stoll(parts[5]);
        e.finalState = static_cast<device::ConnectionState>(std::stoi(parts[6]));
        e.switchReason = parts[7];
        e.errorMessage = parts[8];
        history_.push_back(std::move(e));
    }
    return true;
}

}  // namespace usn::session
