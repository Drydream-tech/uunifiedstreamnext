# Building UnifiedStreamNext

## 1. Core Library & Automated Tests (Linux / Offline Verified)

No external dependencies beyond a C++20 compiler (`g++ >= 12` or `clang++ >= 15`):

```bash
./scripts/build_core_linux.sh
```

Or with CMake + Ninja:

```bash
cmake -S . -B build/core -G Ninja
cmake --build build/core
ctest --test-dir build/core --output-on-failure
```

## 2. Fetching & Pinning Upstream Sources (Requires Internet / Local Cache)

```bash
./scripts/fetch_upstreams.sh
```

This checks `third_party/cache/` first, then clones pinned tags into `third_party/` and writes exact 40-character commit SHAs to `UPSTREAM_LOCKS.md`.

## 3. Windows Integrated Build (Windows 10/11 Workstation)

Prerequisites: Visual Studio 2022 (MSVC C++20), CMake >= 3.24, Ninja, Qt 6.6+ (Quick/QML), Rust (for RustDesk), Windows SDK.

1. Build upstream engines using their native build systems in `third_party/`:
   - Sunshine (`third_party/sunshine`) → copy runtime to `build/windows/engines/sunshine/`
   - Moonlight Qt (`third_party/moonlight-qt`) → copy runtime to `build/windows/engines/moonlight/`
   - RustDesk (`third_party/rustdesk`) → copy runtime to `build/windows/engines/rustdesk/`
2. Build UnifiedStreamNext Windows App:
   ```powershell
   cmake -S . -B build/windows -G Ninja -DUSN_BUILD_WINDOWS_UI=ON -DCMAKE_PREFIX_PATH="C:/Qt/6.7.2/msvc2019_64"
   cmake --build build/windows
   ```

## 4. Android Client & Android Host (Android Studio / Gradle)

Prerequisites: JDK 17+, Android SDK (API 34+), Android NDK, Gradle 8.x.
- Client module: `apps/android/` (Jetpack Compose + Moonlight Android + RustDesk Android adapter)
- Host module: `android-host/` (`MediaProjection` + `MediaCodec` H.264/HEVC encoder + Accessibility input bridge)
