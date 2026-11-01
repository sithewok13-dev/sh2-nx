# Silent Hill 2 on iPhone and iPad

The same port as the Switch build: the PC game's `sh2pc.exe` recompiled to C, running natively. No emulator, so
no JIT is needed. It draws with OpenGL ES 3.0 at the screen's full resolution.

**Status: experimental.** The renderer passes `tests/run-render-test.sh`, and every source compiles for iOS
(`.github/workflows/ios.yml`), but it has not yet been played through on a phone.

## What you need

- Your own PC copy of Silent Hill 2 with the Enhanced Edition packs installed (the same files the Switch build
  needs: at least Enhanced Executable, Essential Files, Image Enhancement Pack and Audio Enhancement Pack).
- A Mac with Xcode, CMake, Ninja and Python 3 (`brew install cmake ninja python`).
- An iPhone or iPad on iOS 15 or later, and a way to install apps yourself: SideStore, AltStore or Sideloadly
  (a free Apple ID works).

## Build

```sh
git clone -b ios https://github.com/sithewok13-dev/sh2-nx
cd sh2-nx
ln -s "/path/to/your/Silent Hill 2" game      # the folder with sh2pc.exe
tools/ios/build.sh
```

The first run turns `sh2pc.exe` into C (a few minutes) and builds SDL2 and FFmpeg for iOS; later runs only
rebuild what changed. The result is `build/ios/SH2.ipa`.

Generated game code and the `.ipa` contain code derived from the game: keep them to yourself.

## Install

1. Install `build/ios/SH2.ipa` with SideStore, AltStore or Sideloadly.
2. Open the Files app: **On My iPhone > Silent Hill 2** appears once the app has been installed (start it once
   if it does not).
3. Copy the *contents* of your game folder there, so that `sh2pc.exe`, `data/` and `sh2e/` sit at the top
   (about 8.4 GB with the Enhanced Edition movies). AirDrop to Files, or the Finder's Files tab with the phone
   plugged in, both work.
4. Start the app. Without the game files it says what is missing.

## Playing

- **Controllers**: Xbox, PlayStation and MFi controllers work as on the Switch build; pair them in Settings >
  Bluetooth first. There are no touch controls yet.
- **Resolution**: *Full screen* (the default) matches the phone's shape, up to 1920 pixels wide, and is scaled
  to the panel's full resolution. The launcher's Options also offer 1080p, 720p and the original 4:3.
- **Logs**: `sh2.log`, and `sh2-crash.log` after a crash, are written next to the game files, so the Files app
  shows them.

## Notes

- The game reserves 4 GB of address space, as on PC. iOS may refuse that much to apps installed with a free
  Apple ID; the port then runs with a smaller guest heap and says so in `sh2.log` (`guest arena ... MB`). If it
  stops with "guest heap exhausted", installing with a paid developer account and the
  `com.apple.developer.kernel.extended-virtual-addressing` entitlement (`ios/SH2.entitlements`) lifts the limit.
- The app stops drawing while it is in the background, as iOS requires, and carries on where it was.
