#include "ConnectionStateMachine.hpp"

namespace usn::connection {

bool ConnectionStateMachine::isValidTransition(device::ConnectionState from, device::ConnectionState to) {
    using S = device::ConnectionState;
    if (from == to) return true;

    switch (from) {
        case S::Offline:
            return to == S::Discovering || to == S::Online || to == S::Connecting || to == S::Failed;
        case S::Discovering:
            return to == S::Online || to == S::Offline || to == S::Failed;
        case S::Online:
            return to == S::Connecting || to == S::Offline || to == S::Discovering;
        case S::Connecting:
            return to == S::Authenticating || to == S::Failed || to == S::Disconnecting || to == S::Reconnecting;
        case S::Authenticating:
            return to == S::Connected || to == S::Failed || to == S::Disconnecting;
        case S::Connected:
            return to == S::Reconnecting || to == S::Disconnecting || to == S::Failed;
        case S::Reconnecting:
            return to == S::Connecting || to == S::Authenticating || to == S::Connected ||
                   to == S::Disconnecting || to == S::Failed;
        case S::Disconnecting:
            return to == S::Offline || to == S::Online || to == S::Failed;
        case S::Failed:
            return to == S::Offline || to == S::Online || to == S::Connecting || to == S::Reconnecting;
    }
    return false;
}

bool ConnectionStateMachine::transitionTo(device::ConnectionState next, const std::string& reason) {
    if (!isValidTransition(state_, next)) {
        return false;
    }
    // Critical invariant: NEVER report Connected based solely on process start or timers
    if (next == device::ConnectionState::Connected && !milestones_.isReadyForConnectedState()) {
        return false;
    }
    state_ = next;
    if (next == device::ConnectionState::Failed) {
        lastError_ = reason;
    } else if (next == device::ConnectionState::Connected || next == device::ConnectionState::Online) {
        lastError_.clear();
    }
    if (next == device::ConnectionState::Offline || next == device::ConnectionState::Disconnecting) {
        milestones_.reset();
    }
    return true;
}

void ConnectionStateMachine::onEngineProcessStarted() {
    milestones_.engineProcessStarted = true;
}

void ConnectionStateMachine::onNetworkConnected() {
    milestones_.networkConnected = true;
    if (state_ == device::ConnectionState::Connecting) {
        transitionTo(device::ConnectionState::Authenticating);
    }
}

void ConnectionStateMachine::onAuthenticated() {
    milestones_.authenticated = true;
}

void ConnectionStateMachine::onSessionEstablished() {
    milestones_.sessionEstablished = true;
}

void ConnectionStateMachine::onMediaStreamStarted() {
    milestones_.mediaActive = true;
    if (milestones_.isReadyForConnectedState() &&
        (state_ == device::ConnectionState::Authenticating || state_ == device::ConnectionState::Reconnecting)) {
        transitionTo(device::ConnectionState::Connected);
    }
}

void ConnectionStateMachine::onInputChannelReady() {
    milestones_.inputReady = true;
    if (milestones_.isReadyForConnectedState() &&
        (state_ == device::ConnectionState::Authenticating || state_ == device::ConnectionState::Reconnecting)) {
        transitionTo(device::ConnectionState::Connected);
    }
}

void ConnectionStateMachine::onFailure(const std::string& error) {
    milestones_.reset();
    transitionTo(device::ConnectionState::Failed, error);
}

void ConnectionStateMachine::reset() {
    state_ = device::ConnectionState::Offline;
    milestones_.reset();
    lastError_.clear();
}

}  // namespace usn::connection
