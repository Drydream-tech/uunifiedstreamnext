#include "LoggingService.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <regex>
#include <sstream>

namespace usn::settings {

LoggingService& LoggingService::instance() {
    static LoggingService inst;
    return inst;
}

void LoggingService::configure(const std::string& logFilePath, LogLevel minLevel) {
    std::lock_guard<std::mutex> lock(mutex_);
    minLevel_ = minLevel;
    if (fileStream_.is_open()) {
        fileStream_.close();
    }
    if (!logFilePath.empty()) {
        fileStream_.open(logFilePath, std::ios::out | std::ios::app);
    }
}

void LoggingService::setMinLevel(LogLevel minLevel) {
    std::lock_guard<std::mutex> lock(mutex_);
    minLevel_ = minLevel;
}

const char* LoggingService::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
    }
    return "INFO";
}

std::string LoggingService::redactSecrets(const std::string& input) {
    static const std::regex kSecretPattern(
        R"(((?:password|passwd|pin|token|secret|api_key|auth_key|private_key|credential)\s*[:=]\s*)([^\s,;\"']+))",
        std::regex::icase);
    return std::regex_replace(input, kSecretPattern, "$1[REDACTED]");
}

void LoggingService::log(LogLevel level, const std::string& component, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (static_cast<int>(level) < static_cast<int>(minLevel_)) {
        return;
    }

    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuf{};
#ifdef _WIN32
    gmtime_s(&tmBuf, &t);
#else
    gmtime_r(&t, &tmBuf);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%dT%H:%M:%SZ")
        << " [" << levelToString(level) << "]"
        << " [" << component << "] "
        << redactSecrets(message);

    std::string line = oss.str();
    if (fileStream_.is_open()) {
        fileStream_ << line << '\n';
        fileStream_.flush();
    }

    if (ringBuffer_.size() >= kMaxRingEntries) {
        ringBuffer_.erase(ringBuffer_.begin());
    }
    ringBuffer_.push_back(std::move(line));
}

std::vector<std::string> LoggingService::recentEntries(size_t maxCount) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (ringBuffer_.size() <= maxCount) {
        return ringBuffer_;
    }
    return std::vector<std::string>(ringBuffer_.end() - static_cast<std::ptrdiff_t>(maxCount), ringBuffer_.end());
}

void LoggingService::clearRecent() {
    std::lock_guard<std::mutex> lock(mutex_);
    ringBuffer_.clear();
}

}  // namespace usn::settings
