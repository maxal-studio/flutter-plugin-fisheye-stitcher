# Fisheye Stitcher

A Dart plugin for stitching dual fisheye images into a spherical (equirectangular) image.

# Features

- Stitch fisheye images with ease
- Set the belding width, in order to fade out the stitching errors
- Auto optimization of the parameters

## Example

```dart
import 'package:fisheye_stitcher/fisheye_stitcher.dart';
```

```dart
StitchingResult stitchingResult =
    await fisheye_stitcher.stitchFisheyeImages(
  fisheye1Parameters: fisheye_stitcher.createFishEyeParams(
    sampleHeight: int, // The height of the image from which the params are generated
    imagePath: image.path,
    radius: int,
    centerX: int,
    centerY: int,
    fov: double,
    /// The transform array is needed only if the camera lenese need correction
    /// Usually they do, as it's almost impossible for the camera factories to align the lenses perfectly
    /// In the transform array you can pass multiple rotations in different order
    /// (This is helpful if you use the optimization function, that will generate new parameters and rotations)
    transform: [
      [ROTATEY, double],
      [ROTATEZ, double],
      [ROTATEX, double],
    ],
  ),
  fisheye2Parameters: fisheye_stitcher.createFishEyeParams(
    sampleHeight: int, // The height of the image from which the params are generated
    imagePath: image.path,
    radius: int,
    centerX: int,
    centerY: int,
    fov: double,
    transform: [
      [ROTATEY, double],
      [ROTATEZ, double],
      [ROTATEX, double],
    ],
  ),
  outputWidth: 2000,
  outputHeight: 0,
  enableDebugging: false,
  /// If the numberOfOptimizationIterations parameter is greater then 1, the script will start an auto-optimization
  /// process. This will take time (The greater the numberOfOptimizationIterations,
  /// the greater the optimized result will be)
  numberOfOptimizationIterations: 1
);
```

To generate the params, use photoshop

![How to generate parameters](./doc/parameters_helper.png)

    - The function requires parameters for 2 fisheye images.
    - You can use 1 image if it has both fisheyes inside.
    - centerX and centerX are the center coordinates in px's of the fisheye image (origin is the top left corner of the image)
    - radius , is the radius of the fisheye
    - fov, is the cameras field of view

## DEMO (Example App)

![DEMO (Example App)](./doc/demo.jpg)

# Developers Documentation

## Clone and run the example app (Android)

The Android build depends on [libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo), which is included as a git submodule at the repo root. Android (and Linux/Windows) compile the native code from source at build time.

After cloning, initialize the submodule:

```bash
git submodule update --init --recursive
```

Then run the example app:

```bash
cd example
flutter pub get
flutter run
```

You also need a working Flutter/Android setup (Android SDK, NDK, and a device or emulator). The first Android build can take a few minutes while libjpeg-turbo is compiled.

## Rebuild the Apple xcframework (iOS & macOS)

On iOS and macOS the plugin ships a **prebuilt dynamic** `fisheye_stitcher.xcframework` (vendored in `ios/` and `macos/`). This matches how other FFI plugins such as `camera_360` work: CocoaPods embeds the framework as-is, so it still loads correctly in host apps that use `use_frameworks! :linkage => :static` (for example when `onnxruntime` is present). Dart resolves symbols via `DynamicLibrary.process()`.

**You only need to rebuild the xcframework when C sources change** — e.g. files under `src/` or `ios/libjpeg-turbo/` (the build script uses the iOS libjpeg-turbo copy for all Apple slices).

### Prerequisites

- macOS with Xcode and the iOS + macOS SDKs installed
- Command-line tools: `xcodebuild`, `clang`, `lipo` (included with Xcode)

### Build

From the plugin root:

```bash
./tool/build_apple_xcframework.sh
```

The script:

1. Compiles `src/bitmaplib.c`, `src/fisheye_stitcher.c`, and the libjpeg-turbo sources into a dynamic library for each slice:
   - **iOS device** — arm64
   - **iOS simulator** — arm64 + x86_64 (universal)
   - **macOS** — arm64 + x86_64 (universal)
2. Runs `dsymutil` on each slice and embeds the dSYM bundles via `xcodebuild -create-xcframework -debug-symbols` (so App Store uploads do not warn about missing `fisheye_stitcher` symbols).
3. Assembles `build/apple/fisheye_stitcher.xcframework`
4. Copies the result into `ios/fisheye_stitcher.xcframework` and `macos/fisheye_stitcher.xcframework`

Commit the updated xcframework(s) in `ios/` and `macos/` together with your C changes so apps and CI pick up the new binary without running the script.

### Verify exports (optional)

After building, confirm the FFI entry points are exported:

```bash
xcrun dyld_info -exports ios/fisheye_stitcher.xcframework/ios-arm64/fisheye_stitcher.framework/fisheye_stitcher \
  | grep -E 'createFishEyeParams|_stitch'
```

You should see `_createFishEyeParams` and `_stitch`.

### Platform summary

| Platform | Native delivery | Rebuild when |
|----------|-----------------|--------------|
| Android / Linux / Windows | Compiled from source (`src/CMakeLists.txt`) | Automatic on `flutter build` |
| iOS / macOS | Prebuilt `fisheye_stitcher.xcframework` | Run `./tool/build_apple_xcframework.sh` after C changes |

## Native CLI (src)

You can run the stitcher app directly from the src folder to test it out.

```bash
cd src
make fisheye_stitcher
./fisheye_stitcher
```

# License

jpegtran_ffi code is licensed under the 3-Clause BSD License.
Please do note that it is a wrapper around libjpeg-turbo - so please see [its license](https://github.com/libjpeg-turbo/libjpeg-turbo/blob/ab6c4a5db10901fa2ae9e8594768175e02b6e016/LICENSE.md)
