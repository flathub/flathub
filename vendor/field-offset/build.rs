extern crate rustc_version;
use rustc_version::{version, version_meta, Channel, Version};

fn main() {
    // Assert we haven't travelled back in time
    assert!(version().unwrap().major >= 1);

    // Check for a minimum version
    if version().unwrap() >= Version::parse("1.36.0").unwrap() {
        println!("cargo:rustc-cfg=fieldoffset_maybe_uninit");
        println!("cargo:rustc-cfg=fieldoffset_has_alloc");
    }

    if version_meta().unwrap().channel == Channel::Nightly {
        println!("cargo:rustc-cfg=fieldoffset_assert_in_const_fn");
    }
}
