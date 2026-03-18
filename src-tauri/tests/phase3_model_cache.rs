use std::{
    path::{Path, PathBuf},
    sync::{
        atomic::{AtomicUsize, Ordering},
        Arc,
    },
};

use openkara_lib::separator::model_cache::ModelCache;

#[test]
fn model_cache_reuses_the_loaded_model_for_the_same_path() {
    let load_count = Arc::new(AtomicUsize::new(0));
    let mut cache = ModelCache::default();
    let model_path = PathBuf::from("/tmp/model-a.onnx");

    {
        let load_count = Arc::clone(&load_count);
        let loaded = cache
            .get_or_load_with(&model_path, move |path: &Path| {
                load_count.fetch_add(1, Ordering::SeqCst);
                Ok::<String, anyhow::Error>(path.display().to_string())
            })
            .expect("first load should succeed");
        assert_eq!(loaded, "/tmp/model-a.onnx");
    }

    {
        let load_count = Arc::clone(&load_count);
        let loaded = cache
            .get_or_load_with(&model_path, move |path: &Path| {
                load_count.fetch_add(1, Ordering::SeqCst);
                Ok::<String, anyhow::Error>(path.display().to_string())
            })
            .expect("cached load should succeed");
        assert_eq!(loaded, "/tmp/model-a.onnx");
    }

    assert_eq!(load_count.load(Ordering::SeqCst), 1);
}

#[test]
fn model_cache_reloads_when_the_model_path_changes() {
    let load_count = Arc::new(AtomicUsize::new(0));
    let mut cache = ModelCache::default();

    let first = Path::new("/tmp/model-a.onnx");
    let second = Path::new("/tmp/model-b.onnx");

    let _ = cache
        .get_or_load_with(first, {
            let load_count = Arc::clone(&load_count);
            move |path: &Path| {
                load_count.fetch_add(1, Ordering::SeqCst);
                Ok::<String, anyhow::Error>(path.display().to_string())
            }
        })
        .expect("first load should succeed");
    let _ = cache
        .get_or_load_with(second, {
            let load_count = Arc::clone(&load_count);
            move |path: &Path| {
                load_count.fetch_add(1, Ordering::SeqCst);
                Ok::<String, anyhow::Error>(path.display().to_string())
            }
        })
        .expect("path change should trigger reload");

    assert_eq!(load_count.load(Ordering::SeqCst), 2);
}
