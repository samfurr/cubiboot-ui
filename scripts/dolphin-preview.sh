#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(dirname "$script_dir")

dolphin_app=${DOLPHIN_APP:-/Applications/Dolphin.app}
dol_path=${1:-"$repo_dir/cubeboot/cubeboot.dol"}
ipl_path=${2:-"${IPL_BIN:-$HOME/Desktop/IPL.bin}"}
profile_dir=${DOLPHIN_USER_DIR:-"${TMPDIR:-/tmp}/cubiboot-dolphin-preview-$UID"}
ipl_region=${DOLPHIN_IPL_REGION:-USA}

if [ ! -d "$dolphin_app" ]; then
    echo "Dolphin was not found at: $dolphin_app" >&2
    exit 1
fi

if [ ! -f "$dol_path" ]; then
    echo "Preview DOL was not found at: $dol_path" >&2
    echo "Build it first with: make preview-build" >&2
    exit 1
fi

if [ ! -f "$ipl_path" ]; then
    echo "GameCube IPL was not found at: $ipl_path" >&2
    echo "Set IPL_BIN or pass the IPL path as the second argument." >&2
    exit 1
fi

if pgrep -f "Dolphin.*cubiboot-dolphin-preview-$UID" >/dev/null 2>&1; then
    echo "A cubiboot Dolphin preview is already running." >&2
    echo "Quit that Dolphin window before launching the rebuilt DOL." >&2
    exit 1
fi

mkdir -p "$profile_dir/GC/$ipl_region" "$profile_dir/Config"
cp "$ipl_path" "$profile_dir/GC/$ipl_region/IPL.bin"

if [ ! -f "$profile_dir/Config/Dolphin.ini" ]; then
    printf '%s\n' \
        '[Analytics]' \
        'Enabled = False' \
        'PermissionAsked = True' \
        > "$profile_dir/Config/Dolphin.ini"
fi

echo "Launching Dolphin with an isolated profile: $profile_dir"
open -na "$dolphin_app" --args -u "$profile_dir" -e "$dol_path"
