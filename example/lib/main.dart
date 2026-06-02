import 'dart:io';
import 'package:fisheye_stitcher/models/stitching_result.dart';
import 'optimized_params.dart';
import 'package:flutter/material.dart';
import 'dart:async';
import 'package:fisheye_stitcher/fisheye_stitcher.dart';
import 'package:image_picker/image_picker.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  StitchingResult? stitchingResult;
  bool stitchingInProgress = false;
  Image? uploadedImage;
  final ImagePicker _picker = ImagePicker();

  // Controllers for fisheye 1 parameters
  final TextEditingController sampleHeight1Controller =
      TextEditingController(text: '3040');
  final TextEditingController radius1Controller =
      TextEditingController(text: '1475');
  final TextEditingController centerX1Controller =
      TextEditingController(text: '1510');
  final TextEditingController centerY1Controller =
      TextEditingController(text: '1517');
  final TextEditingController fov1Controller =
      TextEditingController(text: '191.5');
  final TextEditingController rotateX1Controller =
      TextEditingController(text: '0.6');
  final TextEditingController rotateY1Controller =
      TextEditingController(text: '-90');
  final TextEditingController rotateZ1Controller =
      TextEditingController(text: '1.7');

  // Controllers for fisheye 2 parameters
  final TextEditingController sampleHeight2Controller =
      TextEditingController(text: '3040');
  final TextEditingController radius2Controller =
      TextEditingController(text: '1485');
  final TextEditingController centerX2Controller =
      TextEditingController(text: '4574');
  final TextEditingController centerY2Controller =
      TextEditingController(text: '1520');
  final TextEditingController fov2Controller =
      TextEditingController(text: '194.7');
  final TextEditingController rotateX2Controller =
      TextEditingController(text: '0');
  final TextEditingController rotateY2Controller =
      TextEditingController(text: '90');
  final TextEditingController rotateZ2Controller =
      TextEditingController(text: '0');

  @override
  void initState() {
    super.initState();
  }

  // Method to pick an image from gallery
  Future<void> _pickImage() async {
    try {
      final XFile? image = await _picker.pickImage(source: ImageSource.gallery);
      if (image != null) {
        setState(() {
          uploadedImage = Image.file(File(image.path));
        });

        stitchingResult = await _stitchImage(image);
      }
    } catch (e) {
      debugPrint(e.toString());
    }
  }

  // Method to stitch an image
  Future<StitchingResult> _stitchImage(XFile image) async {
    setState(() {
      stitchingInProgress = true;
    });

    StitchingResult stitchingResult = await stitchFisheyeImages(
      fisheye1Parameters: createFishEyeParams(
        imagePath: image.path,
        sampleHeight: int.parse(sampleHeight1Controller.text),
        radius: int.parse(radius1Controller.text),
        centerX: int.parse(centerX1Controller.text),
        centerY: int.parse(centerY1Controller.text),
        fov: double.parse(fov1Controller.text),
        transform: [
          [ROTATEY, double.parse(rotateY1Controller.text)],
          [ROTATEZ, double.parse(rotateZ1Controller.text)],
          [ROTATEX, double.parse(rotateX1Controller.text)],
        ],
      ),
      fisheye2Parameters: createFishEyeParams(
        imagePath: image.path,
        sampleHeight: int.parse(sampleHeight2Controller.text),
        radius: int.parse(radius2Controller.text),
        centerX: int.parse(centerX2Controller.text),
        centerY: int.parse(centerY2Controller.text),
        fov: double.parse(fov2Controller.text),
        transform: [
          [ROTATEX, double.parse(rotateX2Controller.text)],
          [ROTATEY, double.parse(rotateY2Controller.text)],
          [ROTATEZ, double.parse(rotateZ2Controller.text)],
        ],
      ),
      outputWidth: 4000,
      outputHeight: 2000,
      enableDebugging: false,
      numberOfOptimizationIterations: 1,
    );

    setState(() {
      stitchingInProgress = false;
    });

    return stitchingResult;
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: const Text('Fisheye Stitcher'),
        ),
        body: SingleChildScrollView(
          child: Container(
            padding: const EdgeInsets.all(10),
            child: Column(
              children: [
                Row(
                  children: [
                    // Column for fisheye 1 parameters
                    Expanded(
                      child: Column(
                        children: [
                          TextField(
                            controller: sampleHeight1Controller,
                            decoration: const InputDecoration(
                                labelText: 'Sample Height'),
                            keyboardType: TextInputType.number,
                          ),
                          TextField(
                            controller: radius1Controller,
                            decoration:
                                const InputDecoration(labelText: 'Radius 1'),
                            keyboardType: TextInputType.number,
                          ),
                          TextField(
                            controller: centerX1Controller,
                            decoration:
                                const InputDecoration(labelText: 'Center X 1'),
                            keyboardType: TextInputType.number,
                          ),
                          TextField(
                            controller: centerY1Controller,
                            decoration:
                                const InputDecoration(labelText: 'Center Y 1'),
                            keyboardType: TextInputType.number,
                          ),
                          TextField(
                            controller: fov1Controller,
                            decoration:
                                const InputDecoration(labelText: 'FOV 1'),
                            keyboardType: TextInputType.number,
                          ),
                          TextField(
                            controller: rotateX1Controller,
                            decoration:
                                const InputDecoration(labelText: 'Rotate X 1'),
                            keyboardType: TextInputType.number,
                          ),
                          TextField(
                            controller: rotateY1Controller,
                            decoration:
                                const InputDecoration(labelText: 'Rotate Y 1'),
                            keyboardType: TextInputType.number,
                          ),
                          TextField(
                            controller: rotateZ1Controller,
                            decoration:
                                const InputDecoration(labelText: 'Rotate Z 1'),
                            keyboardType: TextInputType.number,
                          ),
                        ],
                      ),
                    ),
                    // Column for fisheye 2 parameters
                    Expanded(
                      child: Column(
                        children: [
                          TextField(
                            controller: sampleHeight2Controller,
                            decoration: const InputDecoration(
                                labelText: 'Sample Height'),
                            keyboardType: TextInputType.number,
                          ),
                          TextField(
                            controller: radius2Controller,
                            decoration:
                                const InputDecoration(labelText: 'Radius 2'),
                            keyboardType: TextInputType.number,
                          ),
                          TextField(
                            controller: centerX2Controller,
                            decoration:
                                const InputDecoration(labelText: 'Center X 2'),
                            keyboardType: TextInputType.number,
                          ),
                          TextField(
                            controller: centerY2Controller,
                            decoration:
                                const InputDecoration(labelText: 'Center Y 2'),
                            keyboardType: TextInputType.number,
                          ),
                          TextField(
                            controller: fov2Controller,
                            decoration:
                                const InputDecoration(labelText: 'FOV 2'),
                            keyboardType: TextInputType.number,
                          ),
                          TextField(
                            controller: rotateX2Controller,
                            decoration:
                                const InputDecoration(labelText: 'Rotate X 2'),
                            keyboardType: TextInputType.number,
                          ),
                          TextField(
                            controller: rotateY2Controller,
                            decoration:
                                const InputDecoration(labelText: 'Rotate Y 2'),
                            keyboardType: TextInputType.number,
                          ),
                          TextField(
                            controller: rotateZ2Controller,
                            decoration:
                                const InputDecoration(labelText: 'Rotate Z 2'),
                            keyboardType: TextInputType.number,
                          ),
                        ],
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 20),
                ElevatedButton(
                  onPressed: _pickImage, // Button to pick an image
                  child: const Text('Upload Fisheye Image'),
                ),
                const SizedBox(height: 20),
                uploadedImage == null // Display uploaded image
                    ? Container()
                    : Image(
                        image: uploadedImage!.image,
                        height: 200,
                      ),
                if (stitchingInProgress) const CircularProgressIndicator(),
                if (stitchingResult != null)
                  Column(
                    children: [
                      Text("Success: ${stitchingResult!.success}"),
                      Text("Message: ${stitchingResult!.message}"),
                      Text("Image: ${stitchingResult!.image?.path}"),
                      if (stitchingResult!.image != null) // DIsplaye the image
                        Image(
                          height: 200,
                          image: FileImage(
                            stitchingResult!.image as File,
                          ),
                        ),
                      Text(
                          "Optimized Image: ${stitchingResult!.optimizedImage}"),
                      OptimizedParamsWidget(
                          optimizedParams: stitchingResult!.optimizedParams),
                    ],
                  ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
