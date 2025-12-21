// Take a look at the license at the top of the repository in the LICENSE file.

#[cfg(unix)]
#[test]
fn test_content_type_guess() {
    // We only test for directory and file without extension here as we can't guarantee the
    // CI runners will have any mimetypes installed.
    let ret: (glib::GString, bool) =
        gio::functions::content_type_guess(Some(std::path::Path::new("test/")), None);
    assert_eq!(ret.0, "inode/directory");

    let ret: (glib::GString, bool) =
        gio::functions::content_type_guess(Some(std::path::Path::new("test")), None);
    assert_eq!(ret.0, "application/octet-stream");
}
