use anyhow::Result;
use std::path::Path;

#[derive(Debug)]
pub struct ModelCache<T> {
    cached_key: Option<String>,
    cached_model: Option<T>,
}

impl<T> Default for ModelCache<T> {
    fn default() -> Self {
        Self {
            cached_key: None,
            cached_model: None,
        }
    }
}

impl<T> ModelCache<T> {
    pub fn get_or_load_with(
        &mut self,
        path: &Path,
        load: impl FnOnce(&Path) -> Result<T>,
    ) -> Result<&mut T> {
        self.get_or_load_with_key(path.display().to_string(), || load(path))
    }

    pub fn get_or_load_with_key(
        &mut self,
        key: impl Into<String>,
        load: impl FnOnce() -> Result<T>,
    ) -> Result<&mut T> {
        let key = key.into();

        if self.cached_key.as_deref() != Some(key.as_str()) {
            // Demucs model loads are large enough that re-reading from disk for every
            // song dominates batch separation time. The cache stays single-instance
            // on purpose: current separation is sequential, so reuse matters more
            // than parallelism here.
            self.cached_key = None;
            self.cached_model = None;
        }

        if self.cached_model.is_none() {
            self.cached_model = Some(load()?);
            self.cached_key = Some(key);
        }

        Ok(self
            .cached_model
            .as_mut()
            .expect("model cache should hold a model after a successful load"))
    }
}

#[cfg(test)]
mod tests {
    use super::ModelCache;

    #[test]
    fn reloads_when_cache_key_changes_for_same_model_path() {
        let mut cache = ModelCache::default();
        let mut loads = 0;

        let model = cache
            .get_or_load_with_key("/tmp/model.onnx::cpu", || {
                loads += 1;
                Ok::<_, anyhow::Error>(loads)
            })
            .expect("initial model load should succeed");
        assert_eq!(*model, 1);

        let model = cache
            .get_or_load_with_key("/tmp/model.onnx::cpu", || {
                loads += 1;
                Ok::<_, anyhow::Error>(loads)
            })
            .expect("same cache key should reuse the model");
        assert_eq!(*model, 1);

        let model = cache
            .get_or_load_with_key("/tmp/model.onnx::coreml", || {
                loads += 1;
                Ok::<_, anyhow::Error>(loads)
            })
            .expect("different provider key should force a reload");
        assert_eq!(*model, 2);
    }
}
