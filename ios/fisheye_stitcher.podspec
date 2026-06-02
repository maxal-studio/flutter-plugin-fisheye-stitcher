#
# To learn more about a Podspec see http://guides.cocoapods.org/syntax/podspec.html.
# Run `pod lib lint fisheye_stitcher.podspec` to validate before publishing.
#
Pod::Spec.new do |s|
  s.name             = 'fisheye_stitcher'
  s.version          = '0.0.1'
  s.summary          = 'A new Flutter FFI plugin project.'
  s.description      = <<-DESC
A new Flutter FFI plugin project.
                       DESC
  s.homepage         = 'http://example.com'
  s.license          = { :file => '../LICENSE' }
  s.author           = { 'Your Company' => 'email@example.com' }

  # The native C sources (src/ + libjpeg-turbo) are precompiled into a dynamic
  # `fisheye_stitcher.xcframework` by tool/build_apple_xcframework.sh and
  # vendored here. Shipping a dynamic framework (instead of building from
  # source) means CocoaPods embeds it as-is even when the host app uses
  # `use_frameworks! :linkage => :static`, so the FFI symbols are always present
  # in the process for `DynamicLibrary.process()`. Re-run the build script after
  # changing any C source.
  s.source           = { :path => '.' }
  s.vendored_frameworks = 'fisheye_stitcher.xcframework'
  s.dependency 'Flutter'
  s.platform = :ios, '12.0'

  # Flutter.framework does not contain a i386 slice.
  s.pod_target_xcconfig = { 'EXCLUDED_ARCHS[sdk=iphonesimulator*]' => 'i386' }
  s.swift_version = '5.0'
end
