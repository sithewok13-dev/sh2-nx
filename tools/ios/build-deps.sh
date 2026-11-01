#!/bin/sh
# Builds what the iOS app links, static and for arm64 iPhones/iPads, into build/ios-deps:
#   SDL2    window, GL context, input, audio
#   FFmpeg  only what the game needs: the Bink demuxer and decoders (movies, src/win32/bink.c) and the PNG
#           decoder (Enhanced Edition images, src/game/ee.c)
# Needs a Mac with Xcode (or its command line tools), CMake and curl. Run once; tools/ios/build.sh calls it.
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${SH2_IOS_DEPS:-$ROOT/build/ios-deps}
WORK=$ROOT/build/ios-deps-src
MIN_IOS=15.0
SDL_VERSION=2.30.9
FFMPEG_VERSION=7.1
JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)

command -v xcrun >/dev/null || { echo "xcrun not found: this needs a Mac with Xcode"; exit 1; }
SDK=$(xcrun --sdk iphoneos --show-sdk-path)
mkdir -p "$OUT" "$WORK"

fetch() {  # url, archive name
    [ -f "$WORK/$2" ] || curl -fL --retry 3 -o "$WORK/$2" "$1"
}

if [ ! -f "$OUT/lib/libSDL2.a" ]; then
    echo "== SDL2 $SDL_VERSION"
    fetch "https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VERSION/SDL2-$SDL_VERSION.tar.gz" sdl2.tar.gz
    rm -rf "$WORK/SDL2-$SDL_VERSION" && tar -xzf "$WORK/sdl2.tar.gz" -C "$WORK"
    cmake -S "$WORK/SDL2-$SDL_VERSION" -B "$WORK/sdl2-build" -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=$MIN_IOS \
        -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TEST=OFF -DCMAKE_INSTALL_PREFIX="$OUT"
    cmake --build "$WORK/sdl2-build" --config Release -j "$JOBS"
    cmake --install "$WORK/sdl2-build" --config Release
fi

if [ ! -f "$OUT/lib/libavcodec.a" ]; then
    echo "== FFmpeg $FFMPEG_VERSION (Bink and PNG only)"
    fetch "https://ffmpeg.org/releases/ffmpeg-$FFMPEG_VERSION.tar.xz" ffmpeg.tar.xz
    rm -rf "$WORK/ffmpeg-$FFMPEG_VERSION" && tar -xJf "$WORK/ffmpeg.tar.xz" -C "$WORK"
    cd "$WORK/ffmpeg-$FFMPEG_VERSION"
    CC="$(xcrun --sdk iphoneos -f clang)"
    FLAGS="-arch arm64 -isysroot $SDK -mios-version-min=$MIN_IOS"
    ./configure --prefix="$OUT" --enable-cross-compile --target-os=darwin --arch=aarch64 \
        --cc="$CC" --sysroot="$SDK" --extra-cflags="$FLAGS" --extra-ldflags="$FLAGS" \
        --enable-static --disable-shared --disable-programs --disable-doc --disable-network --disable-autodetect \
        --disable-everything --enable-zlib \
        --enable-avformat --enable-avcodec --enable-swscale --enable-swresample \
        --enable-protocol=file --enable-demuxer=bink \
        --enable-decoder=bink,binkaudio_rdft,binkaudio_dct,png
    make -j "$JOBS"
    make install
fi

echo "iOS dependencies are in $OUT"
