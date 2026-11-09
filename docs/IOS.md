# Silent Hill 2 on iPhone and iPad

The same port as the Switch build: the PC game's `sh2pc.exe` recompiled to C, running natively. No emulator, so
no JIT is needed. It draws with OpenGL ES 3.0 at the screen's full resolution, through
[ANGLE](https://chromium.googlesource.com/angle/angle) on Metal (Apple's own OpenGL ES is deprecated and costs far
more time per draw call). Where ANGLE cannot start (iOS 15 to 17, which its build does not support), it draws with
Apple's OpenGL ES instead.

**Status: experimental.** The renderer passes `tests/run-render-test.sh`, and every source compiles for iOS
(`.github/workflows/ios.yml`), but it has not yet been played through on a phone.

## What you need

- Your own PC copy of Silent Hill 2 (Director's Cut / Restless Dreams), with the
  [Enhanced Edition](https://enhanced.townofsilenthill.com/SH2/) setup run on it with at least the **Enhanced
  Executable**, **Essential Files**, **Image Enhancement Pack** and **Audio Enhancement Pack**. The FMV Enhancement
  Pack is optional. The port is built for the `sh2pc.exe` that setup installs: 5,685,248 bytes, SHA-1
  `3201a5e1029f3ae3b77770255dcbe10360b3cd09` (`tools/ios/build.sh` checks it).
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
3. Copy these from your PC install there, so they sit at the top (AirDrop to Files, or the Finder's Files tab
   with the phone plugged in, both work):

   ```text
   sh2pc.exe        the Enhanced Edition's exe (the one checked above)
   data/            the game's data, about 2.4 GB
   sh2e/            the Enhanced Edition packs, about 6 GB with the movies (sh2e/movie), less without them
   sh2e.ini         which packs to use (docs/sh2e.ini.example); the launcher's Options edit it too
   language.ini     optional: SET DX_CONFIG_LANGUAGE 1 is English
   lang/            optional: a translation made for the Enhanced Edition
   keyconf.dat      optional: your PC's button mapping
   ```

   `sh2e/pic/etc` and `sh2e/menu/mc` are needed even with every pack switched off: the title and save screens are
   laid out for the Enhanced Edition's art. Saves go to `data/save`, next to the rest.
4. Start the app. Without the game files it says what is missing.

## Playing

- **Touch controls**: a stick that appears wherever your left thumb lands, A/B/X/Y on the right (Xbox
  colors), dragging elsewhere on the right half for the right stick (the search camera), L1/L2 and R1/R2 in the
  top corners, Back and Start at the top. They follow the Xbox layout the Enhanced Edition uses.
- **Controllers**: Xbox, PlayStation and MFi controllers; pair them in Settings > Bluetooth. While one is
  connected the touch controls hide, and they come back when it disconnects.
- **Resolution**: *Full screen* (the default) matches the phone's shape, up to 1920 pixels wide, and is scaled
  to the panel's full resolution. The launcher's Options also offer 1080p, 720p and the original 4:3.
- **Logs**: `sh2.log`, and `sh2-crash.log` after a crash, are written next to the game files, so the Files app
  shows them. `sh2.log` says which renderer draws (`GL: drawing with ANGLE (Metal)` or `Apple OpenGL ES`), and every
  5 seconds of play how fast the game runs (`frames: 60.0 fps; per frame 16.7 ms ...`): the time per frame split
  into the game's own work, drawing (its draw calls), present (the frame to the screen) and the frame limiter
  (waiting for the next 30 or 60 Hz tick).
- **Renderer**: `renderer=opengles` in `sh2e.ini` draws with Apple's OpenGL ES even where ANGLE works, to compare
  or if ANGLE ever draws something wrong.

## Notes

- The game reserves 4 GB of address space, as on PC. iOS may refuse that much to apps installed with a free
  Apple ID; the port then runs with a smaller guest heap and says so in `sh2.log` (`guest arena ... MB`). If it
  stops with "guest heap exhausted", installing with a paid developer account and the
  `com.apple.developer.kernel.extended-virtual-addressing` entitlement (`ios/SH2.entitlements`) lifts the limit.
- The app stops drawing while it is in the background, as iOS requires, and carries on where it was.
