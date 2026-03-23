fn main() {
    #[cfg(target_os = "macos")]
    {
        // The AirPlay bridge is compiled by the build script, so Cargo must rerun it
        // whenever the Objective-C source changes instead of reusing a stale bridge.
        println!("cargo:rerun-if-changed=src/macos/airplay_bridge.m");
        println!("cargo:rerun-if-changed=src/macos/import_picker.m");
        println!("cargo:rerun-if-changed=src/macos/window_shell.m");

        cc::Build::new()
            .file("src/macos/airplay_bridge.m")
            .file("src/macos/import_picker.m")
            .file("src/macos/window_shell.m")
            .flag("-mmacosx-version-min=11.0")
            .flag("-fobjc-arc")
            .compile("openkara-airplay-bridge");

        println!("cargo:rustc-link-lib=framework=AppKit");
        println!("cargo:rustc-link-lib=framework=AVFoundation");
        println!("cargo:rustc-link-lib=framework=AVKit");
        println!("cargo:rustc-link-lib=framework=AudioToolbox");
        println!("cargo:rustc-link-lib=framework=CoreGraphics");
        println!("cargo:rustc-link-lib=framework=CoreMedia");
        println!("cargo:rustc-link-lib=framework=CoreText");
        println!("cargo:rustc-link-lib=framework=CoreVideo");
        println!("cargo:rustc-link-lib=framework=Foundation");
        println!("cargo:rustc-link-lib=framework=UniformTypeIdentifiers");
    }

    tauri_build::build()
}
