import 'dart:ffi' as ffi;
import 'dart:io';
import "dart:math" show pi;

import 'package:fisheye_stitcher/fisheye_stitcher_bindings_generated.dart';

class OptimizedParams {
  final FisheyeParams fisheye1Params;
  final FisheyeParams fisheye2Params;

  OptimizedParams({
    required this.fisheye1Params,
    required this.fisheye2Params,
  });
}

class Transform {
  final int axis;
  final String label;
  final double value;

  Transform({
    required this.axis,
    required this.label,
    required this.value,
  });
}

class FisheyeParams {
  final String fname;
  final int radius;
  final int centerX;
  final int centerY;
  final double fov;
  final int hflip;
  final int vflip;
  final List<Transform> transforms;

  FisheyeParams({
    required this.fname,
    required this.radius,
    required this.centerX,
    required this.centerY,
    required this.fov,
    required this.hflip,
    required this.vflip,
    required this.transforms,
  });
}

class StitchingResult {
  final bool success;
  final String message;
  final File? image;
  final File? optimizedImage;
  final OptimizedParams optimizedParams;

  StitchingResult({
    required this.success,
    required this.message,
    required this.image,
    required this.optimizedImage,
    required this.optimizedParams,
  });
}

/// Converts a [STITCHING_RESULT] to a [StitchingResult].
///
/// This function takes a [STITCHING_RESULT] and returns a [StitchingResult]
/// with the same information. The [STITCHING_RESULT] is from the C API and
/// the [StitchingResult] is from the Dart API.
StitchingResult convertStitchingResult(STITCHING_RESULT result) {
  return StitchingResult(
    success: result.success,
    message: _convertArrayToString(result.message),
    image: _convertArrayToString(result.image).isNotEmpty
        ? File(_convertArrayToString(result.image))
        : null,
    optimizedImage: _convertArrayToString(result.optimized_image).isNotEmpty
        ? File(_convertArrayToString(result.optimized_image))
        : null,
    optimizedParams: convertOptimizedParams(result.optimized_params),
  );
}

/// Converts an [OPTIMIZED_PARAMS] to an [OptimizedParams].
///
/// This function takes an [OPTIMIZED_PARAMS] and returns an [OptimizedParams]
/// with the same information. The [OPTIMIZED_PARAMS] is from the C API and
/// the [OptimizedParams] is from the Dart API.
OptimizedParams convertOptimizedParams(OPTIMIZED_PARAMS params) {
  return OptimizedParams(
    fisheye1Params: convertFisheyeParams(params.fishey1_params),
    fisheye2Params: convertFisheyeParams(params.fishey2_params),
  );
}

/// Converts a [FISHEYE_PARAMS] to a [FisheyeParams].
///
/// This function takes a [FISHEYE_PARAMS] and returns a [FisheyeParams]
/// with the same information. The [FISHEYE_PARAMS] is from the C API and
/// the [FisheyeParams] is from the Dart API.
FisheyeParams convertFisheyeParams(FISHEYE_PARAMS params) {
  List<Transform> transforms = [];
  for (int i = 0; i < params.ntransform; i++) {
    final transform = (params.transform + i).ref;

    String label = '';
    switch (transform.axis) {
      case 0:
        label = 'ROTATEX';
        break;
      case 1:
        label = 'ROTATEY';
        break;
      case 2:
        label = 'ROTATEZ';
        break;
      default:
        break;
    }

    transforms.add(Transform(
      axis: transform.axis,
      label: label,
      value: transform.value * (180 / pi),
    ));
  }

  return FisheyeParams(
    fname: _convertArrayToString(params.fname, maxLength: 256),
    radius: params.radius,
    centerX: params.centerx,
    centerY: params.centery,
    fov: params.fov,
    hflip: params.hflip,
    vflip: params.vflip,
    transforms: transforms,
  );
}

/// Convert a [ffi.Array] of C characters to a Dart [String].
///
/// The array is assumed to be null-terminated, so the method will stop
/// at the first null character it encounters. The maximum length of the
/// resulting string is [maxLength] characters, if not specified otherwise.
///
/// @param array The array of C characters to convert.
/// @param maxLength The maximum length of the resulting string.
/// @return A Dart [String] containing the characters from the array.
String _convertArrayToString(ffi.Array<ffi.Char> array, {int maxLength = 512}) {
  final List<int> codeUnits = [];

  for (int i = 0; i < maxLength; i++) {
    final int char = array[i];
    if (char == 0) break; // Stop at null terminator
    codeUnits.add(char);
  }

  return String.fromCharCodes(codeUnits);
}
