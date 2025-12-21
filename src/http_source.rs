#[derive(Debug)]
pub struct HttpSource {
    pub inner: reqwest::blocking::Response,
}

impl std::io::Read for HttpSource {
    fn read(&mut self, buf: &mut [u8]) -> std::io::Result<usize> {
        self.inner
            .read(buf)
            .map_err(|e| std::io::Error::new(std::io::ErrorKind::Other, e))
    }
}

impl std::io::Seek for HttpSource {
    fn seek(&mut self, _pos: std::io::SeekFrom) -> std::io::Result<u64> {
        Err(std::io::Error::new(
            std::io::ErrorKind::Unsupported,
            "seeking not supported on HTTP stream",
        ))
    }
}

impl symphonia::core::io::MediaSource for HttpSource {
    fn is_seekable(&self) -> bool {
        false
    }

    fn byte_len(&self) -> Option<u64> {
        None
    }
}
