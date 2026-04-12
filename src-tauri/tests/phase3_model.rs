mod support;

use std::path::PathBuf;

use openkara_lib::{config::ExecutionProviderPreference, separator::model};

fn repo_root() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
}

#[test]
fn resolves_default_demucs_model_path() {
    let model_path = model::default_model_path();

    assert!(model_path.ends_with("src-tauri/models/htdemucs.onnx"));
    assert!(model_path.exists());
}

#[test]
fn resolves_staged_runtime_library_path() {
    let runtime_path = model::default_runtime_library_path();

    assert!(runtime_path.ends_with(format!(
        "src-tauri/{}/{}",
        model::ORT_RUNTIME_STAGING_DIR,
        model::ORT_RUNTIME_FILENAME
    )));
}

#[test]
fn prefers_bundled_runtime_library_path_over_dev_staging() {
    let resource_dir = support::unique_temp_path("phase3-runtime-resource");
    let bundled_dir = resource_dir.join("onnxruntime");
    std::fs::create_dir_all(&bundled_dir).expect("bundled runtime dir should create");
    let bundled_path = bundled_dir.join(model::ORT_RUNTIME_FILENAME);
    std::fs::write(&bundled_path, b"not-a-real-runtime").expect("runtime file should write");

    let resolved = model::resolve_runtime_library_path(Some(&resource_dir))
        .expect("bundled runtime path should resolve");

    assert_eq!(resolved, bundled_path);

    std::fs::remove_file(&bundled_path).expect("runtime file should delete");
    std::fs::remove_dir_all(&resource_dir).expect("runtime resource dir should delete");
}

#[cfg(target_vendor = "apple")]
#[test]
fn resolves_macos_framework_runtime_library_path() {
    let resource_dir = support::unique_temp_path("phase3-runtime-framework")
        .join("OpenKara.app")
        .join("Contents")
        .join("Resources");
    let framework_dir = resource_dir
        .parent()
        .expect("resources should have contents parent")
        .join("Frameworks");
    std::fs::create_dir_all(&framework_dir).expect("framework dir should create");
    let framework_path = framework_dir.join(model::ORT_RUNTIME_FILENAME);
    std::fs::write(&framework_path, b"not-a-real-runtime").expect("framework should write");

    let resolved = model::resolve_runtime_library_path(Some(&resource_dir))
        .expect("framework runtime path should resolve");

    assert_eq!(resolved, framework_path);

    std::fs::remove_file(&framework_path).expect("framework should delete");
    std::fs::remove_dir_all(
        resource_dir
            .ancestors()
            .nth(3)
            .expect("app bundle root should exist"),
    )
    .expect("app bundle root should delete");
}

#[test]
fn loads_embedded_demucs_model_session() {
    let loaded = model::load_from_path(
        &repo_root().join("models").join("htdemucs.onnx"),
        ExecutionProviderPreference::Cpu,
    )
    .expect("demucs model should load");

    assert!(!loaded.inputs.is_empty());
    assert!(!loaded.outputs.is_empty());
}

#[test]
fn fails_with_clear_error_for_missing_model_file() {
    let missing_path = repo_root().join("models").join("missing-model.onnx");
    let error = model::load_from_path(&missing_path, ExecutionProviderPreference::Cpu)
        .expect_err("missing model should fail");

    assert!(error.to_string().contains("missing-model.onnx"));
}

#[test]
fn fails_with_clear_error_for_missing_runtime_library() {
    let missing_dir = support::unique_temp_path("phase3-runtime-missing");
    let missing_staged_path = missing_dir.join(model::ORT_RUNTIME_FILENAME);
    let error = model::resolve_runtime_library_path_for_tests(None, &missing_staged_path)
        .expect_err("missing runtime library should fail");

    assert!(error.to_string().contains("prepare-onnx-runtime"));
}
