// swift-tools-version: 5.8

import PackageDescription

let package = Package(
    name: "quickstart",
    products: [
        .executable(
            name: "quickstart",
            targets: ["quickstart"]
        )
    ],
    dependencies: [
        .package(url: "https://github.com/AparokshaUI/adwaita-swift", from: "0.2.0")
    ],
    targets: [
        .executableTarget(
            name: "quickstart",
            dependencies: [.product(name: "Adwaita", package: "adwaita-swift")]
        )
    ]
)
