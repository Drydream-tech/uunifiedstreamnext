#!/usr/bin/env bash
set -euo pipefail

# Deterministic upstream fetcher for online / local-cache workstations.
# Does not modify global Git configuration.
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TP_DIR="$ROOT_DIR/third_party"
mkdir -p "$TP_DIR"

clone_pinned() {
  local name="$1"
  local url="$2"
  local tag="$3"
  local dest="$TP_DIR/$name"
  if [ -d "$dest/.git" ]; then
    echo "[$name] already present at $dest"
  else
    git clone --depth 1 --branch "$tag" --recurse-submodules "$url" "$dest"
  fi
  local sha
  sha="$(git -C "$dest" rev-parse HEAD)"
  echo "[$name] tag=$tag sha=$sha"
}

clone_pinned "sunshine" "https://github.com/LizardByte/Sunshine.git" "v2026.914.233613"
clone_pinned "moonlight-qt" "https://github.com/moonlight-stream/moonlight-qt.git" "v6.2.0"
clone_pinned "moonlight-android" "https://github.com/moonlight-stream/moonlight-android.git" "v12.2"
clone_pinned "rustdesk" "https://github.com/rustdesk/rustdesk.git" "1.5.3"
