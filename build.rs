fn main() {
    compile_resources(
        &["data"],
        "data/resources/resources.gresource.xml",
        "sysd-manager.gresource",
    );

    compile_schema();
}

// BELOW CODE is COPY of glib-build-tools = "0.19.0"
// THE REASON OF THE COPY IS BECAUSE FEDORA COPR DOESN'T HAVE glib-build-tools

// Take a look at the license at the top of the repository in the LICENSE file.

use std::{
    env, fs,
    path::{Path, PathBuf},
    process::Command,
};

// rustdoc-stripper-ignore-next
/// Call to run `glib-compile-resources` to generate compiled gresources to embed
/// in binary with [`gio::resources_register_include`]. `target` is relative to `OUT_DIR`.
///
/// ```no_run
/// glib_build_tools::compile_resources(
///     &["resources"],
///     "resources/resources.gresource.xml",
///     "compiled.gresource",
/// );
/// ```
pub fn compile_resources<P: AsRef<Path>>(source_dirs: &[P], gresource: &str, target: &str) {
    let out_dir = env::var("OUT_DIR").unwrap();
    let out_dir = Path::new(&out_dir);

    let mut command = Command::new("glib-compile-resources");

    for source_dir in source_dirs {
        command.arg("--sourcedir").arg(source_dir.as_ref());
    }

    let output = command
        .arg("--target")
        .arg(out_dir.join(target))
        .arg(gresource)
        .output()
        .unwrap();

    let path = env::current_dir().expect("env::current_dir() FAIL");
    println!("The current directory is {}", path.display());

    println!("CMD Output: {:#?}", output);

    assert!(
        output.status.success(),
        "glib-compile-resources failed with exit status {} and stderr:\n{}",
        output.status,
        String::from_utf8_lossy(&output.stderr)
    );

    println!("cargo:rerun-if-changed={gresource}");
    let mut command = Command::new("glib-compile-resources");

    for source_dir in source_dirs {
        command.arg("--sourcedir").arg(source_dir.as_ref());
    }

    let output = command
        .arg("--generate-dependencies")
        .arg(gresource)
        .output()
        .unwrap()
        .stdout;
    let output = String::from_utf8(output).unwrap();
    for dep in output.split_whitespace() {
        println!("cargo:rerun-if-changed={dep}");
    }
}

fn compile_schema() {
    const GLIB_SCHEMAS_DIR: &str = ".local/share/glib-2.0/schemas/";
    const GLIB_SCHEMAS_FILE: &str = "data/schemas/io.github.plrigaux.sysd-manager.gschema.xml";

    let path = Path::new(GLIB_SCHEMAS_FILE);
    println!("Path {:?}", path);
    let schema_file = match fs::canonicalize(path) {
        Ok(s) => s,
        Err(e) => {
            println!("Error: {:?}", e);
            return;
        }
    };

    let home_dir = env::var("HOME").unwrap();

    let out_dir = PathBuf::from(home_dir).join(GLIB_SCHEMAS_DIR);

    println!("print out_dir {:?}", out_dir);

    println!("cargo:rerun-if-changed={GLIB_SCHEMAS_FILE}");
    let mut command = Command::new("install");
    let output = command
        .arg("-v")
        .arg("-D")
        .arg(schema_file)
        .arg("-t")
        .arg(&out_dir)
        .output()
        .unwrap();

    println!("Install Schema");
    println!(
        "Install Schema stdout {}",
        String::from_utf8_lossy(&output.stdout)
    );
    println!(
        "Install Schema stderr {}",
        String::from_utf8_lossy(&output.stderr)
    );
    println!("Install Schema status {}", output.status);

    let mut command = Command::new("glib-compile-schemas");
    let output = command.arg(&out_dir).output().unwrap();

    if output.status.success() {
        println!("Compile Schema Done on '{:?}'", out_dir);
    } else {
        println!("Compile Schema Failed on '{:?}'", out_dir);
        println!(
            "Compile Schema stdout {}",
            String::from_utf8_lossy(&output.stdout)
        );
        println!(
            "Compile Schema stderr {}",
            String::from_utf8_lossy(&output.stderr)
        );
        println!("Compile Schema status {}", output.status);
    }
}
