#pragma once

#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace usn::settings {

enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warn = 2,
    Error = 3
};

class LoggingService {
public:
    static LoggingService& instance();

    void configure(const std::string& logFilePath, LogLevel minLevel = LogLevel::Info);
    void setMinLevel(LogLevel minLevel);
    void log(LogLevel level, const std::string& component, const std::string& message);

    static std::string redactSecrets(const std::string& input);

    std::vector<std::string> recentEntries(size_t maxCount = 50) const;
    void clearRecent();

private:
    LoggingService() = default;
    static const char* levelToString(LogLevel level);

    mutable std::mutex mutex_;
    LogLevel minLevel_{LogLevel::Info};
    std::ofstream fileStream_;
    std::vector<std::string> ringBuffer_;
    static constexpr size_t kMaxRingEntries = 500;
};

}  // namespace usn::settings
