# UnifiedStreamNext Architecture

## 1. Layered Module Boundaries

```text
UI Layer (Windows Qt6/QML | Android Jetpack Compose)
    ↓  (Strict boundary: UI never touches upstream engines directly)
DeviceManager (core/device)  +  SettingsManager & LoggingService (core/settings)
    ↓
ConnectionManager (core/connection)  ←→  SessionManager (core/session)
    ↓                                    DiscoveryService (core/discovery)
ConnectionStrategy (Mode A / B / C)      NetworkManager (core/network)
    ↓
IEngineAdapter (engines/IEngineAdapter.hpp)
    ├── SunshineHostManager (engines/sunshine)  [Windows Host only]
    ├── MoonlightAdapter    (engines/moonlight) [LAN P2P Client]
    ├── RustDeskAdapter     (engines/rustdesk)  [Remote Desktop / NAT / Relay]
    └── AndroidHostEngine   (android-host/)     [Android MediaProjection + MediaCodec]
```

## 2. Real 9-State Machine & Milestone Invariants

States: `Offline`, `Discovering`, `Online`, `Connecting`, `Authenticating`, `Connected`, `Reconnecting`, `Disconnecting`, `Failed`.

- **No fake states**: `ConnectionStateMachine::transitionTo(Connected)` rejects any transition unless `networkConnected && authenticated && sessionEstablished && (mediaActive || inputReady)` are all confirmed by real engine/protocol events.
- **Discovery != Online**: Adding a device to `DeviceManager` keeps it `Offline` until `NetworkManager::probeTcpPort` confirms network + service reachability.

## 3. Automatic Connection Strategy

1. **LAN Verification**: `NetworkManager::isSameLan()` enumerates real OS network interfaces (`getifaddrs` on POSIX; `GetAdaptersAddresses` on Windows) and matches `(localIp & netmask) == (targetIp & netmask)` — never naive IP prefix string matching.
2. **Mode A (LAN Streaming)**: Same LAN + Sunshine Host reachable + Moonlight available → `SunshineMoonlight` primary, `RustDesk` fallback.
3. **Mode B (Remote Desktop)**: Different network → `RustDesk` (NAT traversal / rendezvous / relay).
4. **Mode C (LAN Fallback)**: Same LAN, Sunshine unavailable → `RustDesk` LAN fallback.
5. **Android Host**: Never assumes Sunshine runs on Android; routes to independent `AndroidHostEngine` (LAN) or `RustDesk`.
6. **Manual Override**: Per-device `preferredEngine` overrides automatic selection while preserving fallback reason reporting.

## 4. Relay & Security Policy

- **P0 Relay**: `RelayMode::SelfHostedCustom` supports user-configured `hbbs`/`hbbr`/TURN; `RelayMode::OfficialPublicPlaceholder` is reserved and explicitly fails if selected in P0 rather than pretending a public server exists. `lanDirectFirst = true` keeps LAN media P2P.
- **Security**: Relies on upstream pairing/TLS/certificate authentication (Sunshine/Moonlight client certs, RustDesk NaCl/ed25519); stores only certificate fingerprints; `LoggingService::redactSecrets()` strips `password`, `pin`, `token`, `secret`, `key`, and `credential` values before writing logs.
