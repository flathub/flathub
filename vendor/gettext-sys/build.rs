extern crate cc;
extern crate temp_dir;

use std::env;
use std::ffi::OsString;
use std::fs;
use std::io::ErrorKind;
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};
use temp_dir::TempDir;

fn env(name: &str) -> Option<String> {
    let prefix = env::var("TARGET").unwrap().to_uppercase().replace("-", "_");
    let prefixed = format!("{}_{}", prefix, name);
    println!("cargo:rerun-if-env-changed={}", prefixed);

    if let Ok(var) = env::var(&prefixed) {
        return Some(var);
    }

    println!("cargo:rerun-if-env-changed={}", name);
    env::var(name).ok()
}

fn get_windows_gnu_root() -> String {
    // attempt to find the installation directory for the gnu distribution
    env("MSYSTEM_PREFIX")
        .or_else(|| env("MINGW_PREFIX"))
        .or_else(|| {
            // AppVeyor env doesn't declare any usable prefix
            let arch = if env::var("TARGET").unwrap().contains("x86_64") {
                "64"
            } else {
                "32"
            };
            let root_test = PathBuf::from(format!("C:/msys64/mingw{}", arch));
            if root_test.is_dir() {
                Some(root_test.to_str().unwrap().to_owned())
            } else {
                None
            }
        })
        .unwrap_or_else(|| fail("Failed to get gnu installation root dir"))
}

fn posix_path(path: &Path) -> String {
    let path = path
        .to_str()
        .unwrap_or_else(|| fail(&format!("Couldn't convert path {:?} to string", path)));
    if env::var("HOST").unwrap().contains("windows") {
        let path = path.replace("\\", "/");
        if path.find(":") == Some(1) {
            // absolute path with a drive letter
            format!("/{}{}", &path[0..1], &path[2..])
        } else {
            path.to_owned()
        }
    } else {
        path.to_owned()
    }
}

fn check_dependencies(required_programs: Vec<&str>) {
    let command = |x| {
        let status = Command::new("sh")
            .arg("-c")
            .arg(format!("command -v {}", x))
            .status()
            .expect("failed to excute process");

        if status.success() {
            "".to_owned()
        } else {
            format!(" {},", x)
        }
    };

    let errors: String = required_programs.iter().map(|x| command(x)).collect();

    if !errors.is_empty() {
        fail(&format!("The following programs were not found:{}", errors));
    }
}

fn main() {
    let target = env::var("TARGET").unwrap();

    if try_gettext_system() {
        return;
    }

    if target.contains("apple-darwin") {
        println!("cargo:rustc-link-lib=framework=CoreFoundation");
        println!("cargo:rustc-link-lib=dylib=iconv");
    }

    let _ = try_gettext_dir() || try_gettext_dirs() || build_from_source();
}

fn try_gettext_system() -> bool {
    let target = env::var("TARGET").unwrap();

    if cfg!(feature = "gettext-system") || env("GETTEXT_SYSTEM").is_some() {
        if (target.contains("linux") || target.contains("hurd"))
            && (target.contains("-gnu") || target.contains("-musl"))
        {
            // intl is part of glibc and musl
            return true;
        } else if target.contains("windows") && target.contains("-gnu") {
            // gettext doesn't come with a pkg-config file
            let gnu_root = get_windows_gnu_root();
            println!("cargo:rustc-link-search=native={}/lib", &gnu_root);
            println!("cargo:rustc-link-search=native={}/../usr/lib", &gnu_root);
            println!("cargo:rustc-link-lib=dylib=intl");
            // FIXME: should pthread support be optional?
            // It is needed by `cargo test` while generating doc
            println!("cargo:rustc-link-lib=dylib=pthread");
            println!("cargo:include={}/../usr/include", &gnu_root);
            return true;
        } else if target.contains("freebsd") {
            println!("cargo:rustc-link-search=native=/usr/local/lib");
            println!("cargo:rustc-link-lib=dylib=intl");
            return true;
        }
        // else can't use system gettext on this target
    }

    false
}

fn try_gettext_dir() -> bool {
    if let Some(gettext_dir) = env("GETTEXT_DIR") {
        println!("cargo:root={}", gettext_dir);
        if let Some(bin) = env("GETTEXT_BIN_DIR") {
            println!("cargo:bin={}", bin);
        } else {
            println!("cargo:bin={}/bin", gettext_dir);
        }

        if let Some(lib) = env("GETTEXT_LIB_DIR") {
            println!("cargo:lib={}", lib);
            println!("cargo:rustc-link-search=native={}", lib);
        } else {
            println!("cargo:lib={}/lib", gettext_dir);
            println!("cargo:rustc-link-search=native={}/lib", gettext_dir);
        }

        if let Some(include) = env("GETTEXT_INCLUDE_DIR") {
            println!("cargo:include={}", include);
        } else {
            println!("cargo:include={}/include", gettext_dir);
        }

        if env("GETTEXT_STATIC").is_some() {
            println!("cargo:rustc-link-lib=static=intl");
        } else {
            println!("cargo:rustc-link-lib=dylib=intl");
        }

        return true;
    }

    false
}

fn try_gettext_dirs() -> bool {
    if let (Some(bin), Some(lib), Some(include)) = (
        env("GETTEXT_BIN_DIR"),
        env("GETTEXT_LIB_DIR"),
        env("GETTEXT_INCLUDE_DIR"),
    ) {
        println!("cargo:rustc-link-search=native={}", lib);
        if env("GETTEXT_STATIC").is_some() {
            println!("cargo:rustc-link-lib=static=intl");
        } else {
            println!("cargo:rustc-link-lib=dylib=intl");
        }

        println!("cargo:bin={}", bin);
        println!("cargo:lib={}", lib);
        println!("cargo:include={}", include);
        return true;
    }

    false
}

fn build_from_source() -> bool {
    // Programs required to compile GNU gettext
    check_dependencies(vec!["cmp", "diff", "find", "xz", "xzcat"]);

    let target = env::var("TARGET").unwrap();
    let src = env::current_dir().unwrap();
    let build_dir = TempDir::new().unwrap();
    let build_dir = build_dir.path();
    let compiler = cc::Build::new().get_compiler();

    let _ = fs::create_dir(&build_dir.join("build"));
    let _ = fs::create_dir(&build_dir.join("gettext"));

    unpack_tarball(&src, &build_dir);
    run_configure(&target, &compiler, &build_dir);
    run_make(&build_dir);
    run_make_install(&build_dir);
    copy_artifacts_to_out_dir(&build_dir);
    set_up_linking_with_out_dir(&target);

    true
}

fn prepare_cflags(target: &str, compiler: &cc::Tool) -> OsString {
    let mut cflags = OsString::new();
    for arg in compiler.args() {
        cflags.push(arg);
        cflags.push(" ");
    }

    if target.contains("windows") {
        // Avoid undefined reference to `__imp_xmlFree'
        cflags.push("-DLIBXML_STATIC");
    }

    cflags
}

fn unpack_tarball(src: &Path, build_dir: &Path) {
    let xzcat = Command::new("xzcat")
        .arg(&src.join("gettext-0.26.tar.xz"))
        .stdin(Stdio::null())
        .stdout(Stdio::piped())
        .spawn()
        .expect("Failed to execute xzcat. Is it installed?");

    let mut tar = Command::new("tar")
        .arg("xf")
        .arg("-")
        .stdin(Stdio::from(
            xzcat.stdout.expect("Failed to open xzcat's stdout"),
        ))
        .stdout(Stdio::null())
        .current_dir(&build_dir.join("gettext"))
        .spawn()
        .expect("Failed to execute tar. Is it installed?");

    match tar.wait() {
        Err(e) => fail(&format!("tar failed to run: {:?}", e)),
        Ok(exit_status) => {
            if !exit_status.success() {
                fail(&format!("tar returned code {:?}", exit_status.code()))
            }
        }
    }
}

fn run_configure(target: &str, compiler: &cc::Tool, build_dir: &Path) {
    let host = env::var("HOST").unwrap();

    let cflags = prepare_cflags(&target, &compiler);

    let mut cmd = Command::new("sh");
    cmd.env("CC", compiler.path())
        .env("CFLAGS", cflags)
        .env("LD", &which("ld").unwrap())
        .env("VERBOSE", "1")
        .current_dir(&build_dir.join("build"))
        .arg(&posix_path(
            &build_dir
                .join("gettext")
                .join("gettext-0.26")
                .join("gettext-runtime")
                .join("configure"),
        ));

    cmd.arg("--without-emacs");
    cmd.arg("--disable-java");
    cmd.arg("--disable-csharp");
    cmd.arg("--disable-c++");
    cmd.arg("--disable-shared");
    cmd.arg("--enable-static");
    cmd.arg("--enable-fast-install");
    cmd.arg("--with-included-gettext");
    cmd.arg("--with-included-glib");
    cmd.arg("--with-included-libcroco");
    cmd.arg("--with-included-libunistring");

    if target.contains("windows") {
        // FIXME: should pthread support be optional?
        // It is needed by `cargo test` while generating doc
        cmd.arg("--enable-threads=windows");
    }

    cmd.arg(format!("--prefix={}", &posix_path(&build_dir)));
    cmd.arg(format!("--libdir={}", &posix_path(&build_dir.join("lib"))));

    if target != host && (!target.contains("windows") || !host.contains("windows")) {
        // NOTE GNU terminology
        // BUILD = machine where we are (cross) compiling curl
        // HOST = machine where the compiled curl will be used
        // TARGET = only relevant when compiling compilers
        if target.contains("windows") {
            // curl's configure can't parse `-windows-` triples when used
            // as `--host`s. In those cases we use this combination of
            // `host` and `target` that appears to do the right thing.
            cmd.arg(format!("--host={}", host));
            cmd.arg(format!("--target={}", target));
        } else {
            cmd.arg(format!("--build={}", host));
            cmd.arg(format!("--host={}", target));
        }
    }
    run(&mut cmd, "sh");
}

fn run_make(build_dir: &Path) {
    run(
        make()
            .arg(&format!("-j{}", env::var("NUM_JOBS").unwrap()))
            .current_dir(&build_dir.join("build")),
        "make",
    );
}

fn run_make_install(build_dir: &Path) {
    run(
        make().arg("install").current_dir(&build_dir.join("build")),
        "make",
    );
}

fn copy_artifacts_to_out_dir(build_dir: &Path) {
    let dst = PathBuf::from(env::var_os("OUT_DIR").unwrap());

    let mut cmd = Command::new("cp");
    cmd.current_dir(&build_dir)
        .arg("-r")
        .arg(&build_dir.join("bin"))
        .arg(&build_dir.join("include"))
        .arg(&build_dir.join("lib"))
        .arg(&dst);
    run(&mut cmd, "cp");
}

fn set_up_linking_with_out_dir(target: &str) {
    let dst = PathBuf::from(env::var_os("OUT_DIR").unwrap());

    println!("cargo:rustc-link-lib=static=intl");
    println!("cargo:rustc-link-search=native={}/lib", dst.display());
    println!("cargo:lib={}/lib", dst.display());
    println!("cargo:include={}/include", dst.display());
    println!("cargo:bin={}/bin", dst.display());
    println!("cargo:root={}", dst.display());

    if target.contains("windows") {
        println!(
            "cargo:rustc-link-search=native={}/lib",
            &get_windows_gnu_root()
        );
        println!("cargo:rustc-link-lib=dylib=iconv");
    }
}

fn run(cmd: &mut Command, program: &str) {
    println!("running: {:?}", cmd);
    let status = match cmd.status() {
        Ok(status) => status,
        Err(ref e) if e.kind() == ErrorKind::NotFound => {
            fail(&format!(
                "failed to execute command: {}\nis `{}` not installed?",
                e, program
            ));
        }
        Err(e) => fail(&format!("failed to execute command: {}", e)),
    };
    if !status.success() {
        fail(&format!(
            "command did not execute successfully, got: {}",
            status
        ));
    }
}

fn fail(s: &str) -> ! {
    panic!("\n{}\n\nbuild script failed, must exit now", s)
}

fn which(cmd: &str) -> Option<PathBuf> {
    let cmd = format!("{}{}", cmd, env::consts::EXE_SUFFIX);
    let paths = env::var_os("PATH").unwrap();
    env::split_paths(&paths)
        .map(|p| p.join(&cmd))
        .find(|p| fs::metadata(p).is_ok())
}

fn make() -> Command {
    let cmd = if cfg!(target_os = "freebsd") {
        "gmake"
    } else {
        "make"
    };
    let mut cmd = Command::new(cmd);
    // We're using the MSYS make which doesn't work with the mingw32-make-style
    // MAKEFLAGS, so remove that from the env if present.
    if cfg!(windows) {
        cmd.env_remove("MAKEFLAGS").env_remove("MFLAGS");
    }
    return cmd;
}
