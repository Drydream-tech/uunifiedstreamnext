#pragma once

#include <functional>
#include <string>
#include <vector>

namespace usn::engines {

using LineCallback = std::function<void(const std::string& line)>;

// Cross-platform subprocess launcher for isolated engine execution (Moonlight, RustDesk, Sunshine).
// Reads stdout/stderr lines from the child process without blocking UI threads.
class SubprocessRunner {
public:
    SubprocessRunner() = default;
    ~SubprocessRunner();

    SubprocessRunner(const SubprocessRunner&) = delete;
    SubprocessRunner& operator=(const SubprocessRunner&) = delete;

    bool start(const std::string& executable,
               const std::vector<std::string>& args,
               LineCallback onLine,
               std::string& outError);

    void stop();
    bool isRunning() const { return running_; }
    int lastExitCode() const { return exitCode_; }

private:
    bool running_{false};
    int exitCode_{-1};
#ifndef _WIN32
    int pid_{-1};
#else
    void* processHandle_{nullptr};
#endif
};

}  // namespace usn::engines
