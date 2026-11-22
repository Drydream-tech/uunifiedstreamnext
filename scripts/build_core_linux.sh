#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p build/linux-core
g++ -std=c++20 -Wall -Wextra -Werror -O2 \
  core/settings/LoggingService.cpp \
  core/settings/SettingsManager.cpp \
  core/settings/CredentialStore.cpp \
  core/device/DeviceManager.cpp \
  core/network/NetworkManager.cpp \
  core/discovery/DiscoveryService.cpp \
  core/connection/ConnectionStateMachine.cpp \
  core/connection/ConnectionStrategy.cpp \
  core/connection/ConnectionManager.cpp \
  core/session/SessionManager.cpp \
  engines/common/SubprocessRunner.cpp \
  engines/sunshine/SunshineHostManager.cpp \
  engines/moonlight/MoonlightAdapter.cpp \
  engines/rustdesk/RustDeskAdapter.cpp \
  tests/core_tests.cpp \
  -lpthread \
  -o build/linux-core/usn_core_tests

./build/linux-core/usn_core_tests
