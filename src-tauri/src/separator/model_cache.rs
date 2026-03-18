use anyhow::Result;
use std::path::{Path, PathBuf};

#[derive(Debug)]
pub struct ModelCache<T> {
    cached_path: Option<PathBuf>,
    cached_model: Option<T>,
}

impl<T> Default for ModelCache<T> {
    fn default() -> Self {
        Self {
            cached_path: None,
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
        if self.cached_path.as_deref() != Some(path) {
            // Demucs model loads are large enough that re-reading from disk for every
            // song dominates batch separation time. The cache stays single-instance
            // on purpose: current separation is sequential, so reuse matters more
            // than parallelism here.
            self.cached_path = None;
            self.cached_model = None;
        }

        if self.cached_model.is_none() {
            self.cached_model = Some(load(path)?);
            self.cached_path = Some(path.to_path_buf());
        }

        Ok(self
            .cached_model
            .as_mut()
            .expect("model cache should hold a model after a successful load"))
    }
}
