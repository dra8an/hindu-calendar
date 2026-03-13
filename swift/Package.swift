// swift-tools-version: 5.9

import PackageDescription

let package = Package(
    name: "HinduCalendar",
    targets: [
        .target(
            name: "HinduCalendar",
            path: "Sources/HinduCalendar"
        ),
        .executableTarget(
            name: "hindu-calendar",
            dependencies: ["HinduCalendar"],
            path: "Sources/HinduCalendarCLI"
        ),
        .testTarget(
            name: "HinduCalendarTests",
            dependencies: ["HinduCalendar"],
            path: "Tests/HinduCalendarTests"
        ),
    ]
)
