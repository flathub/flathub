import Foundation

let arguments = CommandLine.arguments

// The path to the directory containing the SPM manifest.
let path = arguments.count > 1 ? arguments[1] : "."
// The path to the directory containing the Flatpak manifest.
let pathToManifest = arguments.count > 2 ? arguments[2] : "."
/// An optional suffix.
var suffix = arguments.count > 3 ? arguments[3] : ""

if !suffix.isEmpty {
    suffix = "-" + suffix
}

// Build the Swift package to get a complete list of dependencies under "{path}/.build/workspace-state.json".
let task = Process()
task.executableURL = URL(fileURLWithPath: "/usr/bin/env")
task.arguments = ["swift", "build", "-c", "release", "--package-path", path]
try task.run()
task.waitUntilExit()

// Parse the dependencies in the workspace state file.
let workspaceStatePath = path + "/.build/workspace-state.json"
guard let workspaceStateData = FileManager.default.contents(atPath: workspaceStatePath) else {
    fatalError("The workspace state file expected at \(workspaceStatePath) could not be found.")
}
let workspaceState = try JSONDecoder().decode(WorkspaceState.self, from: workspaceStateData)

// Copy the names of the folders under "{path}/.build/repositories".
let repositoriesPath = path + "/.build/repositories"
let repositoriesContent = try FileManager.default.contentsOfDirectory(atPath: repositoriesPath)

// Generate the JSON file with the sources and the shell script for tweaks.
var content = """
[
"""
var shellContent = """
#!/usr/bin/env bash
mkdir .build/repositories
cd .build/repositories
"""

for dependency in workspaceState.object.dependencies {
    let subpath = dependency.subpath
    content.append("""

        {
            "type": "git",
            "url": "\(dependency.packageRef.location)",
            "disable-shallow-clone": true,
            "commit": "\(dependency.state.checkoutState.revision)",
            "dest": ".build/checkouts/\(subpath)"
        },
    """)
    let folders = repositoriesContent.filter { $0.hasPrefix(subpath + "-") }
    for folder in folders {
        shellContent.append("""

        mkdir ./\(folder)
        cp -r ../checkouts/\(subpath)/.git/* ./\(folder)
        """)
    }
}
content.append("""

    {
         "type": "file",
         "path": "setup-offline\(suffix).sh"
    }
""")
content.append("\n]")

// Save the files.
let pathToSetup = "\(pathToManifest)/setup-offline\(suffix).sh"

let contentData = content.data(using: .utf8)
let shellContentData = shellContent.data(using: .utf8)
try contentData?.write(to: .init(fileURLWithPath: "\(pathToManifest)/generated-sources\(suffix).json"))
try shellContentData?.write(to: .init(fileURLWithPath: pathToSetup))

let executable = Process()
executable.executableURL = URL(fileURLWithPath: "/usr/bin/env")
executable.arguments = ["chmod", "+x", pathToSetup]
try executable.run()
executable.waitUntilExit()

// Types for decoding workspace state file.
struct Dependency: Codable {

    var packageRef: PackageRef
    var state: State
    var subpath: String

    struct PackageRef: Codable {

        var location: String

    }

    struct State: Codable {

        var checkoutState: CheckoutState

    }

    struct CheckoutState: Codable {

        var revision: String

    }

}

struct WorkspaceState: Codable {

    var object: Object

    struct Object: Codable {

        var dependencies: [Dependency]

    }

}
