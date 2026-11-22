# Third-Party License Compliance Inventory

> **Compliance Notice**: Items marked `PENDING LEGAL REVIEW` must be reviewed by qualified counsel before any public or commercial distribution. Personal use does not waive copyleft obligations upon redistribution.

## 1. Component License Matrix

| Component | Source | License | Integration Boundary | Obligations / Risk |
| --- | --- | --- | --- | --- |
| **Sunshine** | [LizardByte/Sunshine](https://github.com/LizardByte/Sunshine) | GPL-3.0 | Bundled standalone host subprocess | Must ship GPL-3.0 text, copyright notices, and corresponding source (including any `third_party/patches/`) when distributing binaries. |
| **Moonlight Qt** | [moonlight-stream/moonlight-qt](https://github.com/moonlight-stream/moonlight-qt) | GPL-3.0-or-later (with MIT, Zlib, LGPL-2.1-or-later subcomponents per [Fedora](https://packages.fedoraproject.org/pkgs/moonlight-qt/moonlight-qt/fedora-44.html)) | Subprocess or GPL-compatible linking | If dynamically/statically linked into `UnifiedStreamNext`, the combined work must comply with GPL-3.0. |
| **Moonlight Android** | [moonlight-stream/moonlight-android](https://github.com/moonlight-stream/moonlight-android) | GPL-3.0 | Android client module / APK integration | Combined APK distribution triggers GPL-3.0 source availability for the integrated Android app. |
| **RustDesk** | [rustdesk/rustdesk](https://github.com/rustdesk/rustdesk) | AGPL-3.0 | **Isolated subprocess via IPC/CLI** | **PENDING LEGAL REVIEW**: Direct linking or code copying triggers AGPL-3.0 across the host app and network-service source disclosure. Subprocess isolation with unmodified or patched-source-provided binaries minimizes contamination risk, but bundled installer distribution still requires counsel review. |
| **Qt 6** | The Qt Company | LGPL-3.0 / Commercial | Dynamic linking (`Qt6Core`, `Qt6Quick`, etc.) | Must dynamically link LGPL-3.0 Qt libraries, allow user relinking, and include LGPL notices (or purchase commercial license). |
| **FFmpeg** | FFmpeg Project | LGPL-2.1+ / GPL-2.0+ (build flags dependent) | Transitive via Sunshine / Moonlight / RustDesk | Verify `--enable-gpl` / `--enable-nonfree` flags; never distribute `--enable-nonfree` binaries. |

## 2. Distribution Checklist (Before Release)

- Include full license texts (`GPL-3.0`, `AGPL-3.0`, `LGPL-3.0`, `MIT`, `Zlib`) and `THIRD_PARTY_NOTICES.txt` in Windows installer and Android APK About screen.
- Publish exact source archives + `third_party/patches/` matching distributed binaries for Sunshine, Moonlight, and RustDesk.
- Resolve **PENDING LEGAL REVIEW**: Confirm whether shipping UnifiedStreamNext under GPL-3.0 / AGPL-3.0 or maintaining strict process-boundary separation satisfies all upstream terms.
