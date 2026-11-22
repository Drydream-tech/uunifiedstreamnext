cmake_minimum_required(VERSION 3.24)
project(UnifiedStreamNext VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(usn_core STATIC
    core/settings/LoggingService.cpp
    core/settings/SettingsManager.cpp
    core/settings/CredentialStore.cpp
    core/device/DeviceManager.cpp
    core/network/NetworkManager.cpp
    core/discovery/DiscoveryService.cpp
    core/connection/ConnectionStateMachine.cpp
    core/connection/ConnectionStrategy.cpp
    core/connection/ConnectionManager.cpp
    core/session/SessionManager.cpp
    engines/common/SubprocessRunner.cpp
    engines/sunshine/SunshineHostManager.cpp
    engines/moonlight/MoonlightAdapter.cpp
    engines/rustdesk/RustDeskAdapter.cpp
)

target_include_directories(usn_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(WIN32)
    target_link_libraries(usn_core PUBLIC ws2_32 iphlpapi)
endif()

enable_testing()
add_executable(usn_core_tests tests/core_tests.cpp)
target_link_libraries(usn_core_tests PRIVATE usn_core)
add_test(NAME usn_core_tests COMMAND usn_core_tests)

option(USN_BUILD_WINDOWS_UI "Build Windows Qt6/QML application" OFF)
if(USN_BUILD_WINDOWS_UI)
    add_subdirectory(apps/windows)
endif()
