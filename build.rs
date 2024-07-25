#[cfg(not(feature = "flutter"))]
use std::process::Command;
#[cfg(not(feature = "flutter"))]
use std::path::PathBuf;

#[cfg(windows)]
fn build_windows() {
    let file = "src/platform/windows.cc";
    cc::Build::new().file(file).compile("windows");
	println!("cargo:rustc-link-lib=WtsApi32");
   println!("cargo:rerun-if-changed={}", file);
}

	
#[cfg(target_os = "macos")]
fn build_mac() {
    let file = "src/platform/macos.mm";
    let mut b = cc::Build::new();
    if let Ok(os_version::OsVersion::MacOS(v)) = os_version::detect() {
        let v = v.version;
        if v.contains("10.14") {
            b.flag("-DNO_InputMonitoringAuthStatus=1");
        }
    }
    b.file(file).compile("macos");
    println!("cargo:rerun-if-changed={}", file);
}

#[cfg(all(windows, feature = "packui"))]
fn build_manifest() {
    use std::io::Write;
//    if std::env::var("PROFILE").unwrap() == "release" {
        let mut res = winres::WindowsResource::new();
        res.set_icon("res/icon.ico")
            .set_language(winapi::um::winnt::MAKELANGID(
                winapi::um::winnt::LANG_ENGLISH,
                winapi::um::winnt::SUBLANG_ENGLISH_US,
            ))
            .set_manifest_file("res/manifest.xml");
        match res.compile() {
            Err(e) => {
                write!(std::io::stderr(), "{}", e).unwrap();
                std::process::exit(1);
            }
            Ok(_) => {}
        }
//    }
}

fn install_android_deps() {
    let target_os = std::env::var("CARGO_CFG_TARGET_OS").unwrap();
    if target_os != "android" {
        return;
    }
    let mut target_arch = std::env::var("CARGO_CFG_TARGET_ARCH").unwrap();
    if target_arch == "x86_64" {
        target_arch = "x64".to_owned();
    } else if target_arch == "x86" {
        target_arch = "x86".to_owned();
    } else if target_arch == "aarch64" {
        target_arch = "arm64".to_owned();
    } else {
        target_arch = "arm".to_owned();
    }
    let target = format!("{}-android", target_arch);
    let vcpkg_root = std::env::var("VCPKG_ROOT").unwrap();
    let mut path: std::path::PathBuf = vcpkg_root.into();
    path.push("installed");
    path.push(target);
    println!(
        "{}",
        format!(
            "cargo:rustc-link-search={}",
            path.join("lib").to_str().unwrap()
        )
    );
    //println!("cargo:rustc-link-lib=ndk_compat");
    println!("cargo:rustc-link-lib=oboe");
    //println!("cargo:rustc-link-lib=oboe_wrapper");
    println!("cargo:rustc-link-lib=c++");
    println!("cargo:rustc-link-lib=OpenSLES");
}


fn main() {
    hbb_common::gen_version();
    install_android_deps();

    #[cfg(all(feature = "packui", ))]
    {
        // Download packfolder if it doesn't exist
        #[cfg(target_os = "linux")]
		let packfolder = "https://github.com/c-smile/sciter-sdk/raw/9f1724a45f5a53c4d513b02ed01cdbdab08fa0e5/bin.lnx/packfolder";
        let output = "target/packfolder";
        let path = PathBuf::from(output);
        #[cfg(target_os = "linux")]
		if !path.exists() {
			Command::new("wget").args([packfolder, "-O", output]).output().expect("wget packfolder failed");
			Command::new("chmod").args(["+x", output]).output().expect("chmod failed");
        }

        // Run packfolder to create target/resources.rc
		if cfg!(target_arch = "arm") || cfg!(target_arch = "aarch64") {

		} else {
			Command::new(path).args(["src/ui", "target/resources.rc", "-i", "*.html;*.css;*.tis", "-v", "resources", "-binary",]).output().expect("packfolder failed!");
		}
    }


    #[cfg(all(windows, feature = "packui"))]
    build_manifest();
    #[cfg(windows)]
    static_vcruntime::metabuild();
    #[cfg(windows)]
    build_windows();
    let target_os = std::env::var("CARGO_CFG_TARGET_OS").unwrap();
    if target_os == "macos" {
        #[cfg(target_os = "macos")]
        build_mac();
        println!("cargo:rustc-link-lib=framework=ApplicationServices");
    }
    println!("cargo:rerun-if-changed=build.rs");
}
