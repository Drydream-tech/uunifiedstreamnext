#pragma once

#include "../device/Device.hpp"

#include <string>

namespace usn::connection {

struct SessionMilestones {
    bool engineProcessStarted{false};
    bool networkConnected{false};
    bool authenticated{false};
    bool sessionEstablished{false};
    bool mediaActive{false};
    bool inputReady{false};

    bool isReadyForConnectedState() const {
        return networkConnected && authenticated && sessionEstablished && (mediaActive || inputReady);
    }

    void reset() {
        engineProcessStarted = false;
        networkConnected = false;
        authenticated = false;
        sessionEstablished = false;
        mediaActive = false;
        inputReady = false;
    }
};

class ConnectionStateMachine {
public:
    ConnectionStateMachine() = default;

    device::ConnectionState state() const { return state_; }
    const SessionMilestones& milestones() const { return milestones_; }
    const std::string& lastError() const { return lastError_; }

    // Enforces strict state transitions and milestone invariants
    bool transitionTo(device::ConnectionState next, const std::string& reason = "");

    // Milestone updates driven by real engine/network events
    void onEngineProcessStarted();
    void onNetworkConnected();
    void onAuthenticated();
    void onSessionEstablished();
    void onMediaStreamStarted();
    void onInputChannelReady();
    void onFailure(const std::string& error);
    void reset();

    static bool isValidTransition(device::ConnectionState from, device::ConnectionState to);

private:
    device::ConnectionState state_{device::ConnectionState::Offline};
    SessionMilestones milestones_;
    std::string lastError_;
};

}  // namespace usn::connection
