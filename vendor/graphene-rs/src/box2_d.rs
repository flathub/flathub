// Take a look at the license at the top of the repository in the LICENSE file.

use std::fmt;

use glib::translate::*;

use crate::{ffi, Box2D, Point, Rect, Vec2};

impl Box2D {
    #[doc(alias = "graphene_box2d_get_vertices")]
    #[doc(alias = "get_vertices")]
    pub fn vertices(&self) -> &[Vec2; 4] {
        unsafe {
            let mut out: [ffi::graphene_vec2_t; 4] = std::mem::zeroed();
            ffi::graphene_box2d_get_vertices(self.to_glib_none().0, &mut out as *mut _);
            &*(&out as *const [ffi::graphene_vec2_t; 4] as *const [Vec2; 4])
        }
    }

    #[doc(alias = "graphene_box2d_to_float")]
    pub fn to_float(&self) -> [f32; 4] {
        unsafe {
            let mut out = std::mem::MaybeUninit::uninit();
            ffi::graphene_box2d_to_float(self.to_glib_none().0, out.as_mut_ptr());
            out.assume_init()
        }
    }

    #[doc(alias = "graphene_box2d_init")]
    pub fn new(min: Option<&Point>, max: Option<&Point>) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut b = Self::uninitialized();
            ffi::graphene_box2d_init(
                b.to_glib_none_mut().0,
                min.to_glib_none().0,
                max.to_glib_none().0,
            );
            b
        }
    }

    #[doc(alias = "graphene_box2d_init_from_points")]
    #[doc(alias = "init_from_points")]
    pub fn from_points(points: &[Point]) -> Self {
        assert_initialized_main_thread!();

        let n = points.len() as u32;

        unsafe {
            let mut b = Self::uninitialized();
            ffi::graphene_box2d_init_from_points(
                b.to_glib_none_mut().0,
                n,
                points.to_glib_none().0,
            );
            b
        }
    }

    #[doc(alias = "graphene_box2d_init_from_vec2")]
    #[doc(alias = "init_from_vec2")]
    pub fn from_vec2(min: Option<&Vec2>, max: Option<&Vec2>) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut b = Self::uninitialized();
            ffi::graphene_box2d_init_from_vec2(
                b.to_glib_none_mut().0,
                min.to_glib_none().0,
                max.to_glib_none().0,
            );
            b
        }
    }

    #[doc(alias = "graphene_box2d_init_from_vectors")]
    #[doc(alias = "init_from_vectors")]
    pub fn from_vectors(vectors: &[Vec2]) -> Self {
        assert_initialized_main_thread!();

        let n = vectors.len() as u32;

        unsafe {
            let mut b = Self::uninitialized();
            ffi::graphene_box2d_init_from_vectors(
                b.to_glib_none_mut().0,
                n,
                vectors.to_glib_none().0,
            );
            b
        }
    }

    #[doc(alias = "graphene_box2d_init_from_rect")]
    pub fn from_rect(src: &Rect) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut b = Self::uninitialized();
            ffi::graphene_box2d_init_from_rect(b.to_glib_none_mut().0, src.to_glib_none().0);
            b
        }
    }
}

impl Default for Box2D {
    fn default() -> Self {
        Self::zero()
    }
}

impl fmt::Debug for Box2D {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Box2D")
            .field("min", &self.min())
            .field("max", &self.max())
            .finish()
    }
}
