#include "ConnectionStrategy.hpp"

namespace usn::connection {

StrategyDecision ConnectionStrategy::evaluate(const device::Device& dev, const StrategyContext& ctx) const {
    StrategyDecision decision;

    // 1. Respect explicit user manual override
    if (dev.preferredEngine != device::EngineType::Auto) {
        decision.mode = ConnectionMode::ManualOverride;
        decision.primaryEngine = dev.preferredEngine;
        if (dev.preferredEngine == device::EngineType::SunshineMoonlight) {
            decision.fallbackEngines = {device::EngineType::RustDesk};
            decision.reason = "User manually selected Sunshine/Moonlight (RustDesk configured as fallback).";
        } else if (dev.preferredEngine == device::EngineType::RustDesk) {
            if (ctx.isSameLan && dev.reachability.sunshineHostAvailable) {
                decision.fallbackEngines = {device::EngineType::SunshineMoonlight};
            }
            decision.reason = "User manually selected RustDesk.";
        } else {
            decision.reason = "User manually selected engine override.";
        }
        return decision;
    }

    // 2. Android Host target cannot run Sunshine directly
    if (dev.type == device::DeviceType::AndroidDevice) {
        if (dev.reachability.androidHostAvailable && ctx.isSameLan) {
            decision.mode = ConnectionMode::ModeA_LanStreaming;
            decision.primaryEngine = device::EngineType::AndroidHost;
            decision.fallbackEngines = {device::EngineType::RustDesk};
            decision.reason = "Target is Android device on same LAN; prioritizing AndroidHost engine with RustDesk fallback.";
            return decision;
        }
        decision.mode = ConnectionMode::ModeB_RemoteDesktop;
        decision.primaryEngine = device::EngineType::RustDesk;
        decision.reason = "Target is Android device; using RustDesk remote desktop engine.";
        return decision;
    }

    // 3. Mode A: Same LAN + Sunshine Host available -> prioritize Sunshine/Moonlight P2P
    if (ctx.isSameLan && dev.reachability.sunshineHostAvailable && ctx.moonlightEngineInstalled) {
        decision.mode = ConnectionMode::ModeA_LanStreaming;
        decision.primaryEngine = device::EngineType::SunshineMoonlight;
        if (dev.reachability.rustDeskAvailable || !dev.rustDeskPeerId.empty()) {
            decision.fallbackEngines.push_back(device::EngineType::RustDesk);
        }
        decision.reason = "Same LAN verified via interface netmask and Sunshine Host is reachable; selecting Mode A (Sunshine/Moonlight P2P).";
        return decision;
    }

    // 4. Mode C: Same LAN, but Sunshine unavailable -> RustDesk LAN fallback
    if (ctx.isSameLan) {
        decision.mode = ConnectionMode::ModeC_LanFallback;
        decision.primaryEngine = device::EngineType::RustDesk;
        decision.reason = "Same LAN detected, but Sunshine Host or Moonlight client is unavailable; selecting Mode C (RustDesk LAN fallback).";
        return decision;
    }

    // 5. Mode B: Remote network -> RustDesk (NAT traversal / rendezvous / relay)
    decision.mode = ConnectionMode::ModeB_RemoteDesktop;
    decision.primaryEngine = device::EngineType::RustDesk;
    decision.reason = "Remote network detected; selecting Mode B (RustDesk remote desktop with NAT traversal/relay).";
    return decision;
}

}  // namespace usn::connection
