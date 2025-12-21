// Take a look at the license at the top of the repository in the LICENSE file.

#![cfg_attr(docsrs, feature(doc_cfg))]
#![doc = include_str!("../README.md")]

pub use glib;
pub use graphene_sys as ffi;

// Graphene has no runtime to initialize
macro_rules! assert_initialized_main_thread {
    () => {};
}

// No-op
macro_rules! skip_assert_initialized {
    () => {};
}

mod auto;

pub mod prelude;

pub use crate::auto::*;

#[cfg(feature = "v1_12")]
#[cfg_attr(docsrs, doc(cfg(feature = "v1_12")))]
mod box2_d;
mod box_;
mod euler;
mod frustum;
mod matrix;
mod plane;
mod point;
mod point3_d;
mod quad;
mod quaternion;
mod ray;
mod rect;
mod size;
mod sphere;
mod triangle;
mod vec2;
mod vec3;
mod vec4;
