import 'package:fisheye_stitcher/models/stitching_result.dart';
import 'package:flutter/material.dart';

class OptimizedParamsWidget extends StatelessWidget {
  final OptimizedParams optimizedParams;

  const OptimizedParamsWidget({super.key, required this.optimizedParams});

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Optimized Parameters',
            ),
            const SizedBox(height: 16),
            _buildFisheyeParams('Fisheye 1', optimizedParams.fisheye1Params),
            const SizedBox(height: 16),
            _buildFisheyeParams('Fisheye 2', optimizedParams.fisheye2Params),
          ],
        ),
      ),
    );
  }

  Widget _buildFisheyeParams(String title, FisheyeParams params) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(title, style: const TextStyle(fontWeight: FontWeight.bold)),
        Text('Filename: ${params.fname}'),
        Text('Radius: ${params.radius}'),
        Text('Center: (${params.centerX}, ${params.centerY})'),
        Text('FOV: ${params.fov.toStringAsFixed(2)}'),
        Text('HFlip: ${params.hflip}'),
        Text('VFlip: ${params.vflip}'),
        const SizedBox(height: 8),
        const Text('Transforms:',
            style: TextStyle(fontWeight: FontWeight.bold)),
        ...params.transforms.map((transform) => Padding(
              padding: const EdgeInsets.only(left: 16.0),
              child: Text(
                'Axis: ${transform.axis} ,'
                'Label: ${transform.label} ,'
                'Value: ${transform.value.toStringAsFixed(2)}',
              ),
            )),
      ],
    );
  }
}
