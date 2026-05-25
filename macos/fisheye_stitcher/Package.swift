// swift-tools-version: 5.9
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "fisheye_stitcher",
    platforms: [
        .macOS("10.11"),
    ],
    products: [
        // Flutter links this product as "fisheye-stitcher" (underscores become hyphens).
        .library(name: "fisheye-stitcher", type: .dynamic, targets: ["fisheye_stitcher"]),
    ],
    targets: [
        .target(
            name: "fisheye_stitcher",
            path: ".",
            exclude: [
                "libjpeg-turbo/jstdhuff.c",
                "libjpeg-turbo/jdmrgext.c",
                "libjpeg-turbo/jdmrg565.c",
                "libjpeg-turbo/jccolext.c",
                "libjpeg-turbo/jdcolext.c",
                "libjpeg-turbo/jdcol565.c",
                "libjpeg-turbo/LICENSE.md",
            ],
            sources: [
                "Sources/fisheye_stitcher",
                "libjpeg-turbo",
            ],
            publicHeadersPath: "Sources/fisheye_stitcher/include/fisheye_stitcher",
            cSettings: [
                .headerSearchPath("Sources/fisheye_stitcher/include/fisheye_stitcher"),
                .headerSearchPath("libjpeg-turbo"),
                .headerSearchPath("src"),
            ]
        ),
    ]
)
