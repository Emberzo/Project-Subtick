#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE5="${UE5_ROOT:-$HOME/projects/UE5}"
EDITOR="$UE5/Engine/Build/BatchFiles/Linux/RunUAT.sh"
if [[ ! -x "$EDITOR" ]]; then
  echo "Set UE5_ROOT to your UnrealEngine checkout (default tried: $UE5)" >&2
  exit 1
fi
# Shipping client package for Linux
"$EDITOR" BuildCookRun \
  -project="$ROOT/ProjectSubtick.uproject" \
  -noP4 -platform=Linux -clientconfig=Shipping -cook -allmaps -stage -pak -archive \
  -archivedirectory="$ROOT/Build/Packages/Linux"

echo "Archived to $ROOT/Build/Packages/Linux"
