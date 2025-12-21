#[cfg(any(target_os = "macos", target_os = "ios"))]
fn build_macos() {
    if std::env::var("TARGET").unwrap().contains("-apple") {
        println!("cargo:rustc-link-lib=framework=MediaPlayer");
    }
}

fn main() {
    #[cfg(any(target_os = "macos", target_os = "ios"))]
    build_macos();
}
