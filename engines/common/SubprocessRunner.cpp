#include "SubprocessRunner.hpp"

#include <cstring>
#include <sstream>

#ifndef _WIN32
#include <cerrno>
#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#else
#include <windows.h>
#endif

namespace usn::engines {

SubprocessRunner::~SubprocessRunner() {
    stop();
}

bool SubprocessRunner::start(const std::string& executable,
                             const std::vector<std::string>& args,
                             LineCallback onLine,
                             std::string& outError) {
    stop();
    if (executable.empty()) {
        outError = "Executable path is empty";
        return false;
    }

#ifndef _WIN32
    int pipefd[2]{-1, -1};
    if (::pipe(pipefd) != 0) {
        outError = std::string("pipe() failed: ") + std::strerror(errno);
        return false;
    }

    pid_t pid = ::fork();
    if (pid < 0) {
        ::close(pipefd[0]);
        ::close(pipefd[1]);
        outError = std::string("fork() failed: ") + std::strerror(errno);
        return false;
    }

    if (pid == 0) {
        ::close(pipefd[0]);
        ::dup2(pipefd[1], STDOUT_FILENO);
        ::dup2(pipefd[1], STDERR_FILENO);
        ::close(pipefd[1]);

        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(executable.c_str()));
        for (const auto& a : args) {
            argv.push_back(const_cast<char*>(a.c_str()));
        }
        argv.push_back(nullptr);
        ::execv(executable.c_str(), argv.data());
        _exit(127);
    }

    ::close(pipefd[1]);
    pid_ = pid;
    running_ = true;

    std::string pending;
    char buf[512];
    ssize_t n = 0;
    while ((n = ::read(pipefd[0], buf, sizeof(buf))) > 0) {
        pending.append(buf, static_cast<size_t>(n));
        size_t pos = 0;
        while ((pos = pending.find('\n')) != std::string::npos) {
            std::string line = pending.substr(0, pos);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (onLine) onLine(line);
            pending.erase(0, pos + 1);
        }
    }
    if (!pending.empty() && onLine) {
        onLine(pending);
    }
    ::close(pipefd[0]);

    int status = 0;
    if (::waitpid(pid_, &status, 0) > 0 && WIFEXITED(status)) {
        exitCode_ = WEXITSTATUS(status);
    }
    running_ = false;
    pid_ = -1;
    return exitCode_ == 0;
#else
    SECURITY_ATTRIBUTES sa{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE readPipe = nullptr, writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &sa, 0)) {
        outError = "CreatePipe failed";
        return false;
    }
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

    std::ostringstream cmd;
    cmd << "\"" << executable << "\"";
    for (const auto& a : args) {
        cmd << " \"" << a << "\"";
    }
    std::string cmdLine = cmd.str();

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = writePipe;
    si.hStdError = writePipe;

    PROCESS_INFORMATION pi{};
    if (!CreateProcessA(nullptr, cmdLine.data(), nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(readPipe);
        CloseHandle(writePipe);
        outError = "CreateProcessA failed";
        return false;
    }
    CloseHandle(writePipe);
    CloseHandle(pi.hThread);
    processHandle_ = pi.hProcess;
    running_ = true;

    std::string pending;
    char buf[512];
    DWORD bytesRead = 0;
    while (ReadFile(readPipe, buf, sizeof(buf), &bytesRead, nullptr) && bytesRead > 0) {
        pending.append(buf, bytesRead);
        size_t pos = 0;
        while ((pos = pending.find('\n')) != std::string::npos) {
            std::string line = pending.substr(0, pos);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (onLine) onLine(line);
            pending.erase(0, pos + 1);
        }
    }
    if (!pending.empty() && onLine) onLine(pending);
    CloseHandle(readPipe);

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    exitCode_ = static_cast<int>(code);
    CloseHandle(pi.hProcess);
    processHandle_ = nullptr;
    running_ = false;
    return exitCode_ == 0;
#endif
}

void SubprocessRunner::stop() {
#ifndef _WIN32
    if (running_ && pid_ > 0) {
        ::kill(pid_, SIGTERM);
        int status = 0;
        ::waitpid(pid_, &status, 0);
        pid_ = -1;
        running_ = false;
    }
#else
    if (running_ && processHandle_) {
        TerminateProcess(static_cast<HANDLE>(processHandle_), 1);
        CloseHandle(static_cast<HANDLE>(processHandle_));
        processHandle_ = nullptr;
        running_ = false;
    }
#endif
}

}  // namespace usn::engines
