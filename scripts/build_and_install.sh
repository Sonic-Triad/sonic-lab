#!/bin/zsh
# Build Audio Units, ad-hoc sign them, install them for the current user and validate with auval.
#
#   scripts/build_and_install.sh                  # every plugin in plugins/
#   scripts/build_and_install.sh TriadOverdrive   # just the named plugin folder(s)
set -euo pipefail

ROOT="${0:A:h:h}"
BUILD_DIR="$ROOT/build"
DEST="$HOME/Library/Audio/Plug-Ins/Components"

if (( $# > 0 )); then
    plugins=("$@")
else
    plugins=("$ROOT"/plugins/*/CMakeLists.txt(N:h:t))
fi

for plugin in "${plugins[@]}"; do
    [[ -f "$ROOT/plugins/$plugin/CMakeLists.txt" ]] || { echo "No plugin folder: plugins/$plugin" >&2; exit 1; }
done

if [[ ! -f "$ROOT/external/JUCE/CMakeLists.txt" ]]; then
    git -C "$ROOT" submodule update --init
fi

cmake -S "$ROOT" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --target "${plugins[@]/%/_AU}"

mkdir -p "$DEST"
components=()

for plugin in "${plugins[@]}"; do
    built=("$BUILD_DIR/plugins/$plugin/${plugin}_artefacts/Release/AU/"*.component)
    codesign --force --deep --sign - "$built[1]"

    installed="$DEST/${built[1]:t}"
    rm -rf "$installed"
    cp -R "$built[1]" "$DEST/"
    components+=("$installed")
    echo "Installed $installed"
done

# Make the system rescan installed Audio Units
killall -9 AudioComponentRegistrar 2>/dev/null || true

failed=0

for component in "${components[@]}"; do
    plist="$component/Contents/Info.plist"
    type=$(/usr/libexec/PlistBuddy -c "Print :AudioComponents:0:type" "$plist")
    subtype=$(/usr/libexec/PlistBuddy -c "Print :AudioComponents:0:subtype" "$plist")
    manufacturer=$(/usr/libexec/PlistBuddy -c "Print :AudioComponents:0:manufacturer" "$plist")

    name="${component:t:r}"
    log="$BUILD_DIR/auval-${name// /_}.log"

    if auval -v "$type" "$subtype" "$manufacturer" > "$log" 2>&1; then
        echo "auval PASSED: $name ($type $subtype $manufacturer)"
    else
        echo "auval FAILED: $name ($type $subtype $manufacturer), see $log" >&2
        failed=1
    fi
done

exit $failed
