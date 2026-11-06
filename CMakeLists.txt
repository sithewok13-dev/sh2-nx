cmake_minimum_required(VERSION 3.20)
project(sh2nx C)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

file(GLOB GEN CONFIGURE_DEPENDS src/recomp/gen/*.c)
file(GLOB RT CONFIGURE_DEPENDS src/runtime/*.c src/win32/*.c src/d3d8/*.c src/game/*.c)

add_executable(sh2 ${GEN} ${RT})
target_include_directories(sh2 PRIVATE src/recomp/gen)
# Lifted code relies on wrapping arithmetic and type punning through guest memory.
target_compile_options(sh2 PRIVATE -fwrapv -fno-strict-aliasing)
set_source_files_properties(${GEN} PROPERTIES COMPILE_OPTIONS "-w")
# The launcher's images are built into the binary (src/d3d8/launcher.c).
file(SHA1 ${CMAKE_SOURCE_DIR}/assets/splash.png SPLASH_SHA1)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS assets/splash.png)
set_source_files_properties(src/d3d8/launcher.c PROPERTIES COMPILE_DEFINITIONS
                            "SRC_DIR=\"${CMAKE_SOURCE_DIR}\";SPLASH_SHA1=\"${SPLASH_SHA1}\""
                            OBJECT_DEPENDS "${CMAKE_SOURCE_DIR}/assets/splash.png;${CMAKE_SOURCE_DIR}/docs/banner.png;${CMAKE_SOURCE_DIR}/assets/font.png")

# The movies (src/win32/bink.c): FFmpeg's Bink decoders. On Fedora: dnf install ffmpeg-free-devel.
# iOS links its own static FFmpeg instead (tools/ios/build-deps.sh).
if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libswscale libswresample libavutil)
  target_link_libraries(sh2 PRIVATE PkgConfig::FFMPEG)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
  # An iPhone/iPad app (tools/ios/build.sh): OpenGL ES 3.0, SDL2 and a minimal FFmpeg, both static, from
  # tools/ios/build-deps.sh. The bundle is unsigned: SideStore, AltStore or Sideloadly sign it on install.
  set(SH2_IOS_DEPS "${CMAKE_SOURCE_DIR}/build/ios-deps" CACHE PATH "SDL2 and FFmpeg built for iOS (tools/ios/build-deps.sh)")
  enable_language(OBJC)
  target_sources(sh2 PRIVATE src/runtime/ios_photos.m)
  target_compile_definitions(sh2 PRIVATE SH2_IOS SH2_GLES)
  target_include_directories(sh2 PRIVATE ${SH2_IOS_DEPS}/include)
  foreach(lib SDL2main SDL2 avformat avcodec swscale swresample avutil)
    target_link_libraries(sh2 PRIVATE ${SH2_IOS_DEPS}/lib/lib${lib}.a)
  endforeach()
  target_link_libraries(sh2 PRIVATE z iconv m
    "-framework AVFoundation" "-framework AudioToolbox" "-framework CoreAudio" "-framework CoreBluetooth"
    "-framework CoreFoundation" "-framework CoreGraphics" "-framework CoreHaptics" "-framework CoreMotion"
    "-framework Foundation" "-framework GameController" "-framework Metal" "-framework OpenGLES"
    "-framework QuartzCore" "-framework UIKit")
  set(SH2_IOS_BUNDLE_ID "com.example.sh2-nx" CACHE STRING "The app's bundle identifier")
  set(SH2_IOS_VERSION "1.03")
  set(SH2_IOS_BUILD "1" CACHE STRING "The build number (CFBundleVersion): the cloud build passes its run number")
  target_compile_definitions(sh2 PRIVATE SH2_VERSION_LABEL="${SH2_IOS_VERSION} (${SH2_IOS_BUILD})")
  configure_file(ios/Info.plist.in ${CMAKE_BINARY_DIR}/Info.plist @ONLY)
  set_target_properties(sh2 PROPERTIES
    OUTPUT_NAME SH2
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_BINARY_DIR}/Info.plist)
elseif(NINTENDO_SWITCH)
  # Built with the devkitPro toolchain (tools/build-switch.sh); OpenGL comes through EGL and mesa.
  target_include_directories(sh2 PRIVATE ${DEVKITPRO}/portlibs/switch/include)
  target_link_libraries(sh2 PRIVATE SDL2 EGL glapi drm_nouveau stdc++ pthread m)
  # The release NRO: no log (log=1, nxlink), call trace or debugging hooks. -DSH2_LEAN=OFF brings them back.
  option(SH2_LEAN "Switch build without logs, traces or debugging hooks" ON)
  if(SH2_LEAN)
    target_compile_definitions(sh2 PRIVATE SH2_LEAN RECOMP_NO_ICALL_TRACE)
  endif()
  nx_generate_nacp(sh2.nacp NAME "Silent Hill 2" AUTHOR "sh2-nx" VERSION "1.02")
  nx_create_nro(sh2 OUTPUT sh2-nx.nro NACP sh2.nacp NOICON)
else()
  # Desktop GL is loaded at run time through SDL (src/d3d8/gl.c), so nothing links libGL.
  # SH2_GLES draws with OpenGL ES 3.0 instead, linked directly: what iOS has, and testable on Linux through mesa.
  option(SH2_GLES "Draw with OpenGL ES 3.0 (the iOS renderer)" OFF)
  find_package(SDL2 REQUIRED)
  target_link_libraries(sh2 PRIVATE SDL2::SDL2 m pthread)
  if(SH2_GLES)
    target_compile_definitions(sh2 PRIVATE SH2_GLES)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLES REQUIRED IMPORTED_TARGET glesv2)
    target_link_libraries(sh2 PRIVATE PkgConfig::GLES)
  endif()
endif()
