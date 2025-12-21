#[cfg(feature = "icon")]
fn main() {
    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=data");

    const APP_ID: &str = "io.github.noobping.listenmoe";
    glib_build_tools::compile_resources(
        &["data"],
        &format!("data/{APP_ID}.resources.xml"),
        "compiled.gresource",
    );
    #[cfg(target_os = "windows")]
    {
        let ico_path = std::path::Path::new("data").join(format!("{APP_ID}.ico"));
        println!("cargo:rerun-if-changed={}", ico_path.display());
        let mut res = winresource::WindowsResource::new();
        res.set_icon(ico_path.to_string_lossy().as_ref());
        res.compile().expect("Failed to compile Windows resources");
    }
}

#[cfg(not(feature = "icon"))]
fn main() {}
