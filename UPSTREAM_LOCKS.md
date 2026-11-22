# Upstream Version Locks

All upstream repositories are isolated under `third_party/` (never mixed with `core/`). Engineering patches live separately in `third_party/patches/` with documented scope.

| Upstream Project | Official Repository | Pinned Tag | Commit SHA | License | Status |
| --- | --- | --- | --- | --- | --- |
| **Sunshine** | [LizardByte/Sunshine](https://github.com/LizardByte/Sunshine) | `v2026.914.233613` | `PENDING_ONLINE_PIN` (run `scripts/fetch_upstreams.sh` to lock full 40-char SHA) | GPL-3.0 | Tag verified; clone blocked offline |
| **Moonlight Qt** | [moonlight-stream/moonlight-qt](https://github.com/moonlight-stream/moonlight-qt) | `v6.2.0` | `de2467e433821664cdd2224aad8c89a625be1ad9` ([Flathub manifest](https://github.com/flathub/com.moonlight_stream.Moonlight/blob/master/com.moonlight_stream.Moonlight.json)) | GPL-3.0-or-later | Tag & full SHA verified |
| **Moonlight Android** | [moonlight-stream/moonlight-android](https://github.com/moonlight-stream/moonlight-android) | `v12.2` | `PENDING_ONLINE_PIN` (run `scripts/fetch_upstreams.sh` to lock full 40-char SHA) | GPL-3.0 | Tag verified; clone blocked offline |
| **RustDesk** | [rustdesk/rustdesk](https://github.com/rustdesk/rustdesk) | `1.5.3` | `95cca3f` (short SHA on [GitHub Releases](https://github.com/rustdesk/rustdesk/releases); lock full 40-char SHA on clone) | AGPL-3.0 | Tag & short SHA verified |

## Dependency Acquisition Priority

1. Verified local source cache (`third_party/cache/`)
2. Official release tarballs with SHA-256 verification
3. Official upstream Git repositories (exact tag checkout, never unpinned `main`/`master`)

Never modify global Git configuration; all fetch operations use repository-local commands.
