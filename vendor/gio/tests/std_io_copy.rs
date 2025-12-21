use gio::prelude::*;

#[test]
fn std_io_copy_with_gio() {
    let bytes = glib::Bytes::from_owned([1, 2, 3]);
    let mut read = gio::MemoryInputStream::from_bytes(&bytes).into_read();
    let mut write = gio::MemoryOutputStream::new_resizable().into_write();

    let result = std::io::copy(&mut read, &mut write);

    let out_stream = write.into_output_stream();
    out_stream.close(gio::Cancellable::NONE).unwrap();
    assert_eq!(result.unwrap(), 3);
    assert_eq!(out_stream.steal_as_bytes(), bytes);
}
