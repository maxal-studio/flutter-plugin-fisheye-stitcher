#!/usr/bin/env bash
#
# Builds a dynamic `fisheye_stitcher.xcframework` containing iOS device,
# iOS simulator and macOS slices, then copies it into ios/ and macos/ where the
# podspecs vendor it.
#
# Why: the host app links with `use_frameworks! :linkage => :static` (required
# by onnxruntime). A source-built pod would be statically merged into the app
# binary, which breaks the plugin's `DynamicLibrary.open(...framework...)` and
# requires per-app linker hacks. Shipping a prebuilt *dynamic* framework (like
# the camera_360 plugin) sidesteps all of that: CocoaPods embeds vendored
# dynamic frameworks as-is regardless of the host's linkage, so the symbols are
# in the process and `DynamicLibrary.process()` resolves them everywhere.
#
# Re-run this whenever the C sources under src/ or the libjpeg-turbo copy change.
set -euo pipefail

NAME="fisheye_stitcher"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="$ROOT/src"
JPEG_DIR="$ROOT/ios/libjpeg-turbo"   # identical to macos/libjpeg-turbo
BUILD_DIR="$ROOT/build/apple"
STAGE_DIR="$BUILD_DIR/frameworks"

IOS_MIN="12.0"
MACOS_MIN="10.13"

# libjpeg-turbo files that are #included by sibling translation units and must
# NOT be compiled standalone (mirrors `exclude_files` in the podspecs).
EXCLUDE_BASENAMES=(jstdhuff jdmrgext jdmrg565 jccolext jdcolext jdcol565)

is_excluded() {
  local base="$1"
  for e in "${EXCLUDE_BASENAMES[@]}"; do
    [ "$base" = "$e" ] && return 0
  done
  return 1
}

# Collect the compilation unit list once.
SOURCES=("$SRC_DIR/bitmaplib.c" "$SRC_DIR/fisheye_stitcher.c")
for f in "$JPEG_DIR"/*.c; do
  base="$(basename "$f" .c)"
  is_excluded "$base" || SOURCES+=("$f")
done

INCLUDES=(-I"$SRC_DIR" -I"$JPEG_DIR")

# Compile + link a single-arch dynamic library.
#   $1 sdk (iphoneos|iphonesimulator|macosx)
#   $2 clang -target triple
#   $3 output dylib path
build_arch_dylib() {
  local sdk="$1" triple="$2" out_dylib="$3"
  local sysroot objdir objs=()
  sysroot="$(xcrun --sdk "$sdk" --show-sdk-path)"
  objdir="$BUILD_DIR/obj/$(basename "$(dirname "$out_dylib")")-$(basename "$out_dylib")"
  rm -rf "$objdir"; mkdir -p "$objdir"

  for s in "${SOURCES[@]}"; do
    local o="$objdir/$(basename "$s").o"
    xcrun --sdk "$sdk" clang \
      -target "$triple" -isysroot "$sysroot" \
      -O2 -g -fPIC -fvisibility=default -Wno-everything \
      "${INCLUDES[@]}" -c "$s" -o "$o"
    objs+=("$o")
  done

  xcrun --sdk "$sdk" clang \
    -target "$triple" -isysroot "$sysroot" \
    -dynamiclib -install_name "@rpath/$NAME.framework/$NAME" \
    -g \
    -o "$out_dylib" "${objs[@]}"
}

# Produce a dSYM bundle for App Store crash symbolication. Required because the
# plugin ships a prebuilt dynamic framework; without dSYMs Xcode warns during
# upload that the archive is missing symbols for fisheye_stitcher.framework.
make_dsym() {
  local binary="$1" dsym_dir="$2"
  rm -rf "$dsym_dir"
  xcrun dsymutil "$binary" -o "$dsym_dir"
}

write_info_plist() {
  local dest="$1" min_os_key="$2" min_os_val="$3"
  cat > "$dest" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>en</string>
	<key>CFBundleExecutable</key>
	<string>$NAME</string>
	<key>CFBundleIdentifier</key>
	<string>com.maxal.fisheyeStitcher</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>$NAME</string>
	<key>CFBundlePackageType</key>
	<string>FMWK</string>
	<key>CFBundleShortVersionString</key>
	<string>1.0</string>
	<key>CFBundleVersion</key>
	<string>1</string>
	<key>$min_os_key</key>
	<string>$min_os_val</string>
</dict>
</plist>
PLIST
}

# Assemble a flat (iOS-style) framework bundle from a finished binary.
make_flat_framework() {
  local fw="$1" binary="$2"
  rm -rf "$fw"; mkdir -p "$fw/Headers"
  cp "$binary" "$fw/$NAME"
  cp "$SRC_DIR/fisheye_stitcher.h" "$fw/Headers/$NAME.h"
  cp "$SRC_DIR/bitmaplib.h" "$fw/Headers/bitmaplib.h"
  write_info_plist "$fw/Info.plist" "MinimumOSVersion" "$IOS_MIN"
}

# Assemble a versioned (macOS-style) framework bundle from a finished binary.
make_versioned_framework() {
  local fw="$1" binary="$2"
  rm -rf "$fw"
  mkdir -p "$fw/Versions/A/Headers" "$fw/Versions/A/Resources"
  cp "$binary" "$fw/Versions/A/$NAME"
  cp "$SRC_DIR/fisheye_stitcher.h" "$fw/Versions/A/Headers/$NAME.h"
  cp "$SRC_DIR/bitmaplib.h" "$fw/Versions/A/Headers/bitmaplib.h"
  write_info_plist "$fw/Versions/A/Resources/Info.plist" "LSMinimumSystemVersion" "$MACOS_MIN"
  ln -sf "A" "$fw/Versions/Current"
  ln -sf "Versions/Current/$NAME" "$fw/$NAME"
  ln -sf "Versions/Current/Headers" "$fw/Headers"
  ln -sf "Versions/Current/Resources" "$fw/Resources"
}

echo "==> Cleaning build dirs"
rm -rf "$BUILD_DIR"
mkdir -p "$STAGE_DIR"

echo "==> Building iOS device (arm64)"
build_arch_dylib iphoneos "arm64-apple-ios$IOS_MIN" "$BUILD_DIR/ios-device-arm64.dylib"
make_flat_framework "$STAGE_DIR/ios-arm64/$NAME.framework" "$BUILD_DIR/ios-device-arm64.dylib"
make_dsym "$STAGE_DIR/ios-arm64/$NAME.framework/$NAME" "$STAGE_DIR/ios-arm64/$NAME.framework.dSYM"

echo "==> Building iOS simulator (arm64 + x86_64)"
build_arch_dylib iphonesimulator "arm64-apple-ios$IOS_MIN-simulator" "$BUILD_DIR/ios-sim-arm64.dylib"
build_arch_dylib iphonesimulator "x86_64-apple-ios$IOS_MIN-simulator" "$BUILD_DIR/ios-sim-x86_64.dylib"
lipo -create "$BUILD_DIR/ios-sim-arm64.dylib" "$BUILD_DIR/ios-sim-x86_64.dylib" \
  -output "$BUILD_DIR/ios-sim.dylib"
make_flat_framework "$STAGE_DIR/ios-sim/$NAME.framework" "$BUILD_DIR/ios-sim.dylib"
make_dsym "$STAGE_DIR/ios-sim/$NAME.framework/$NAME" "$STAGE_DIR/ios-sim/$NAME.framework.dSYM"

echo "==> Building macOS (arm64 + x86_64)"
build_arch_dylib macosx "arm64-apple-macos$MACOS_MIN" "$BUILD_DIR/macos-arm64.dylib"
build_arch_dylib macosx "x86_64-apple-macos$MACOS_MIN" "$BUILD_DIR/macos-x86_64.dylib"
lipo -create "$BUILD_DIR/macos-arm64.dylib" "$BUILD_DIR/macos-x86_64.dylib" \
  -output "$BUILD_DIR/macos.dylib"
make_versioned_framework "$STAGE_DIR/macos/$NAME.framework" "$BUILD_DIR/macos.dylib"
make_dsym "$STAGE_DIR/macos/$NAME.framework/$NAME" "$STAGE_DIR/macos/$NAME.framework.dSYM"

echo "==> Assembling xcframework (with dSYMs)"
XCFW="$BUILD_DIR/$NAME.xcframework"
rm -rf "$XCFW"
xcodebuild -create-xcframework \
  -framework "$STAGE_DIR/ios-arm64/$NAME.framework" \
  -debug-symbols "$STAGE_DIR/ios-arm64/$NAME.framework.dSYM" \
  -framework "$STAGE_DIR/ios-sim/$NAME.framework" \
  -debug-symbols "$STAGE_DIR/ios-sim/$NAME.framework.dSYM" \
  -framework "$STAGE_DIR/macos/$NAME.framework" \
  -debug-symbols "$STAGE_DIR/macos/$NAME.framework.dSYM" \
  -output "$XCFW"

echo "==> Publishing into ios/ and macos/"
for dest in "$ROOT/ios" "$ROOT/macos"; do
  rm -rf "$dest/$NAME.xcframework"
  cp -R "$XCFW" "$dest/$NAME.xcframework"
done

echo "==> Done. Slices:"
ls -1 "$XCFW"
