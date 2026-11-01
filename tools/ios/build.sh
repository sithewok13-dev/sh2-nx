#!/bin/sh
# Builds the iOS app from your own game: build/ios/SH2.ipa, unsigned, for SideStore, AltStore or Sideloadly.
#
#   1. Put your PC Silent Hill 2 install, with the Enhanced Edition packs, in game/ (see docs/IOS.md).
#   2. tools/ios/build.sh
#
# Generates the game's C from game/sh2pc.exe the first time (tools/regen.sh, a few minutes), builds SDL2 and
# FFmpeg for iOS the first time (tools/ios/build-deps.sh), then the app. Needs a Mac with Xcode, CMake,
# Ninja and Python 3.
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
BUILD=$ROOT/build/ios

[ -f game/sh2pc.exe ] || { echo "game/sh2pc.exe not found: copy or link your game folder to game/ (docs/IOS.md)"; exit 1; }
# The port is made for one build of the exe: the one the Enhanced Edition setup installs.
WANT=3201a5e1029f3ae3b77770255dcbe10360b3cd09
HAVE=$(shasum -a 1 game/sh2pc.exe | cut -d' ' -f1)
if [ "$HAVE" != "$WANT" ]; then
    echo "game/sh2pc.exe is not the one this port is built for (SHA-1 $HAVE, want $WANT)."
    echo "Run the Enhanced Edition setup with the Enhanced Executable pack on your PC install, then copy it again (docs/IOS.md)."
    exit 1
fi
if [ ! -x .venv/bin/python3 ]; then
    python3 -m venv .venv
    .venv/bin/pip install -q -r requirements.txt
fi
if ! ls src/recomp/gen/*.c >/dev/null 2>&1; then
    echo "== generating the game's C from game/sh2pc.exe"
    tools/regen.sh
fi
tools/ios/build-deps.sh

echo "== the app"
cmake -S . -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
    -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO ${SH2_IOS_BUNDLE_ID:+-DSH2_IOS_BUNDLE_ID=$SH2_IOS_BUNDLE_ID}
cmake --build "$BUILD"

rm -rf "$BUILD/Payload" "$BUILD/SH2.ipa"
mkdir -p "$BUILD/Payload"
cp -R "$BUILD/SH2.app" "$BUILD/Payload/"
(cd "$BUILD" && zip -qr SH2.ipa Payload)
rm -rf "$BUILD/Payload"
echo "built $BUILD/SH2.ipa: install it with SideStore, AltStore or Sideloadly, then copy the game files (docs/IOS.md)"
