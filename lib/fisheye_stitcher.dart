import 'dart:ffi';
import 'dart:ffi' as ffi;
import 'package:ffi/ffi.dart';
import 'package:fisheye_stitcher/models/stitching_result.dart';
import 'dart:io';
import 'dart:async';
import 'dart:isolate';
import 'package:path_provider/path_provider.dart';

import 'fisheye_stitcher_bindings_generated.dart';

// ignore: constant_identifier_names
const int ROTATEX = XTILT;

// ignore: constant_identifier_names
const int ROTATEY = YROLL;

// ignore: constant_identifier_names
const int ROTATEZ = ZPAN;

/// Create a FISHEYE_PARAMS structure from the given parameters.
///
/// This function creates a FISHEYE_PARAMS structure from the given parameters.
///
/// The `transform` parameter is a list of tuples, where each tuple contains an axis
/// (XTILT, YROLL or ZPAN) and a value (the amount of rotation in radians).
///
/// The `hFlip` and `vFlip` parameters control whether the fisheye image should be
/// flipped horizontally and vertically, respectively.
///
/// @param imagePath The filename of the fisheye image.
/// @param radius The radius of the fisheye image.
/// @param centerX The x coordinate of the center of the fisheye image.
/// @param centerY The y coordinate of the center of the fisheye image.
/// @param fov The field of view of the fisheye image.
/// @param transform An array of axis and value pairs, describing the transformations
/// to apply to the fisheye image.
/// @param hFlip Whether to flip the fisheye image horizontally.
/// @param vFlip Whether to flip the fisheye image vertically.
///
/// @return A FISHEYE_PARAMS structure, which contains the given parameters.
FISHEYE_PARAMS createFishEyeParams({
  /// The height of the image from which the params are generated.
  required int sampleHeight,

  /// imagePath The filename of the fisheye image.
  required String imagePath,

  /// radius The radius of the fisheye image.
  required int radius,

  /// centerX The x coordinate of the center of the fisheye image.
  required int centerX,

  /// centerY The y coordinate of the center of the fisheye image.
  required int centerY,

  /// fov The field of view of the fisheye image.
  required double fov,

  /// transform An array of axis and value pairs, describing the transformations
  /// to apply to the fisheye image.
  List<List<dynamic>> transform = const [],

  /// hFlip Whether to flip the fisheye image horizontally.
  int hFlip = 1,

  /// vFlip Whether to flip the fisheye image vertically.
  int vFlip = 1,
}) {
  // Convert the image path to a native C string
  final fname = imagePath.toNativeUtf8().cast<ffi.Char>();

  // Allocate memory for a contiguous 2D array of doubles
  final transformValues = malloc<ffi.Double>(transform.length * 2);

  // Populate the 2D array with axis and value pairs
  for (int i = 0; i < transform.length; i++) {
    transformValues[i * 2] =
        (transform[i][0] as int).toDouble(); // Axis (convert int to double)
    transformValues[i * 2 + 1] = transform[i][1] as double; // Value
  }

  // Call the native C function through the _bindings instance
  final params = _bindings.createFishEyeParams(
    fname,
    radius,
    centerX,
    centerY,
    fov,
    hFlip,
    vFlip,
    transformValues.cast(), // Pass the contiguous memory block to C
    transform.length,
    sampleHeight,
  );

  // Free allocated memory
  calloc.free(fname);
  calloc.free(transformValues);

  return params;
}

/// Stitches two fisheye images together into a spherical image.
///
/// The function takes two [FISHEYE_PARAMS] structures representing the
/// parameters of the fisheye images to stitch. It also takes a [String] for
/// the output file path, and various other parameters for configuration.
///
/// - [fisheye1Parameters]: The parameters of the first fisheye image.
///
/// - [fisheye2Parameters]: The parameters of the second fisheye image.
///
/// - [outputWidth]: The width of the output image.
///
/// - [outputHeight]: The height of the output image.
///
/// - [antiAliasLevel]: The level of antialiasing to use.
///
/// - [blendWidth]: The width of the blending zone in degrees.
///
/// - [blendPower]: The power to use for the blending zone.
///
/// - [blendMidpoint]: The midpoint of the blending zone.
///
/// - [numberOfOptimizationIterations]: The number of optimization iterations to perform.
///
/// - [deltafov]: The max amount to perturb the field of view for each optimization iteration.
///
/// - [deltacenter]: The max amount to perturb the center for each optimization iteration.
///
/// - [deltatheta]: The max amount to perturb the rotation for each optimization iteration.
///
/// - [applyInverseCorrection]: Whether to apply the inverse correction or not.
///
/// - [enableDebugging]: Whether to enable debugging output or not.
///
/// Returns a [StitchingResult] structure, which contains the success flag
/// image string, optimized_image string and optimized_parameters string.
/// If the success flag is true, the image string will
/// contain the filename of the output image.
///
/// If [numberOfOptimizationIterations] is greater than 1, this function will try
/// to optimize the parameters using the given number of iterations.
/// The optimized_parameters string will contain the filename of the optimized
/// parameters. The optimized_image string will also contain the filename of the
/// optimized image.
///
/// This function internally calls the native `stitch` function in
/// `src/fisheye_stitcher.c`.
Future<StitchingResult> stitchFisheyeImages({
  /// The first fisheye parameters.
  required FISHEYE_PARAMS fisheye1Parameters,

  /// The second fisheye parameters.
  required FISHEYE_PARAMS fisheye2Parameters,

  /// The width of the output image. (Set to 0 if height is set)
  required int outputWidth,

  /// The height of the output image. (Set to 0 if width is set)
  required int outputHeight,

  /// The amount of antialiasing to use.
  int antiAliasLevel = 1,

  /// The longitude for blending.
  double blendWidth = 10,

  /// The power to use for the blend zone.
  double blendPower = 1,

  /// Mid point for blending
  double blendMidpoint = 180,

  /// The number of optimisation iterations to perform.
  int numberOfOptimizationIterations = 1,

  /// The max amount to perturb the field of view for each optimisation iteration.
  double deltafov = 10,

  /// The max amount to perturb the center for each optimisation iteration.
  int deltacenter = 20,

  /// The max amount to perturb the rotation for each optimisation iteration.
  double deltatheta = 5,

  /// Whether to apply the inverse correction.
  bool applyInverseCorrection = false,

  /// Whether to enable debugging output.
  bool enableDebugging = false,
}) async {
  final SendPort helperIsolateSendPort = await _helperIsolateSendPort;
  final int requestId = _nextStitchingRequestId++;

  // ApplicationDocumentsDirectory is needed to know on what directory does each
  // OS allows us to save files
  final directory = await getApplicationDocumentsDirectory();
  // Convert Dart string to Utf8
  Pointer<Char> imagePathPointer = directory.path.toNativeUtf8().cast<Char>();

  // Make sure only width or height is set
  if (outputHeight != 0) {
    outputWidth = 0;
  }

  final _StitchingRequest request = _StitchingRequest(
    requestId,
    fisheye1Parameters,
    fisheye2Parameters,
    imagePathPointer,
    outputWidth,
    outputHeight,
    antiAliasLevel,
    blendWidth,
    blendPower,
    blendMidpoint,
    numberOfOptimizationIterations,
    deltafov,
    deltacenter,
    deltatheta,
    applyInverseCorrection,
    enableDebugging,
  );
  final Completer<StitchingResult> completer = Completer<StitchingResult>();
  _stitchingRequests[requestId] = completer;
  helperIsolateSendPort.send(request);
  return completer.future;
}

const String _libName = 'fisheye_stitcher';

String _appleSpmFrameworkPath(String libName) {
  // Flutter SPM replaces "_" with "-" in the library product name.
  final frameworkName =
      libName.contains('_') ? libName.replaceAll('_', '-') : libName;
  return '$frameworkName.framework/$frameworkName';
}

/// The dynamic library in which the symbols for [FisheyestitcherBindings] can be found.
final DynamicLibrary _dylib = () {
  if (Platform.isMacOS || Platform.isIOS) {
    for (final path in [
      '$_libName.framework/$_libName', // CocoaPods
      _appleSpmFrameworkPath(_libName), // Swift Package Manager
    ]) {
      try {
        return DynamicLibrary.open(path);
      } on ArgumentError {
        // Try the next known Apple packaging layout.
      }
    }
    return DynamicLibrary.process();
  }
  if (Platform.isAndroid || Platform.isLinux) {
    return DynamicLibrary.open('lib$_libName.so');
  }
  if (Platform.isWindows) {
    return DynamicLibrary.open('$_libName.dll');
  }
  throw UnsupportedError('Unknown platform: ${Platform.operatingSystem}');
}();

/// The bindings to the native functions in [_dylib].
final FisheyestitcherBindings _bindings = FisheyestitcherBindings(_dylib);

/// A response with the result of `stitch`.
///
/// Typically sent from one isolate to another.
class _StitchingResponse {
  final int id;
  final StitchingResult result;

  const _StitchingResponse(this.id, this.result);
}

/// A request to compute `stitch`.
///
/// Typically sent from one isolate to another.
class _StitchingRequest {
  final int id;
  final FISHEYE_PARAMS fisheye1Parameters;
  final FISHEYE_PARAMS fisheye2Parameters;
  final Pointer<Char> outputPath;
  final int outputWidth;
  final int outputHeight;
  final int antiAliasLevel;
  final double blendWidth;
  final double blendPower;
  final double blendMidpoint;
  final int numberOfOptimizationIterations;
  final double deltafov;
  final int deltacenter;
  final double deltatheta;
  final bool applyInverseCorrection;
  final bool enableDebugging;
  const _StitchingRequest(
    this.id,
    this.fisheye1Parameters,
    this.fisheye2Parameters,
    this.outputPath,
    this.outputWidth,
    this.outputHeight,
    this.antiAliasLevel,
    this.blendWidth,
    this.blendPower,
    this.blendMidpoint,
    this.numberOfOptimizationIterations,
    this.deltafov,
    this.deltacenter,
    this.deltatheta,
    this.applyInverseCorrection,
    this.enableDebugging,
  );
}

/// Counter to identify [_StitchingRequest]s and [_StitchingResponse]s.
int _nextStitchingRequestId = 0;

/// Mapping from [_StitchingRequest] `id`s to the completers corresponding to the correct future of the pending request.
final Map<int, Completer<StitchingResult>> _stitchingRequests =
    <int, Completer<StitchingResult>>{};

/// The SendPort belonging to the helper isolate.
Future<SendPort> _helperIsolateSendPort = () async {
  // The helper isolate is going to send us back a SendPort, which we want to
  // wait for.
  final Completer<SendPort> completer = Completer<SendPort>();

  // Receive port on the main isolate to receive messages from the helper.
  // We receive two types of messages:
  // 1. A port to send messages on.
  // 2. Responses to requests we sent.
  final ReceivePort receivePort = ReceivePort()
    ..listen((dynamic data) {
      if (data is SendPort) {
        // The helper isolate sent us the port on which we can sent it requests.
        completer.complete(data);
        return;
      }
      if (data is _StitchingResponse) {
        // The helper isolate sent us a response to a request we sent.
        final Completer<StitchingResult> completer =
            _stitchingRequests[data.id]!;
        _stitchingRequests.remove(data.id);
        completer.complete(data.result);
        return;
      }
      throw UnsupportedError('Unsupported message type: ${data.runtimeType}');
    });

  // Start the helper isolate.
  await Isolate.spawn((SendPort sendPort) async {
    final ReceivePort helperReceivePort = ReceivePort()
      ..listen((dynamic data) async {
        // On the helper isolate listen to requests and respond to them.
        if (data is _StitchingRequest) {
          final STITCHING_RESULT result = _bindings.stitch(
            data.fisheye1Parameters,
            data.fisheye2Parameters,
            data.outputPath,
            data.outputWidth,
            data.outputHeight,
            data.antiAliasLevel,
            data.blendWidth,
            data.blendPower,
            data.blendMidpoint,
            data.numberOfOptimizationIterations,
            data.deltafov,
            data.deltacenter,
            data.deltatheta,
            data.applyInverseCorrection,
            data.enableDebugging,
          );

          final _StitchingResponse response =
              _StitchingResponse(data.id, convertStitchingResult(result));
          sendPort.send(response);
          return;
        }
        throw UnsupportedError('Unsupported message type: ${data.runtimeType}');
      });

    // Send the port to the main isolate on which we can receive requests.
    sendPort.send(helperReceivePort.sendPort);
  }, receivePort.sendPort);

  // Wait until the helper isolate has sent us back the SendPort on which we
  // can start sending requests.
  return completer.future;
}();
