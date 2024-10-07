// The Swift Programming Language
// https://docs.swift.org/swift-book

import Adwaita

@main
struct QuickStart: App {

    let id = "org.flatpak.quickstart"
    var app: GTUIApp!

    var scene: Scene {
        Window(id: "main") { _ in
            Text("Hello, world!")
                .padding(50)
                .topToolbar {
                    HeaderBar.empty()
                }
        }
        .title("Demo")
        .resizable(false)
        .closeShortcut()
        .quitShortcut()
    }

}
