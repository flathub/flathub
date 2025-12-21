// Take a look at the license at the top of the repository in the LICENSE file.

use std::fmt;

use glib::translate::*;

use crate::{ffi, Point, Quad, Rect};

impl Quad {
    #[doc(alias = "graphene_quad_init")]
    pub fn new(p1: &Point, p2: &Point, p3: &Point, p4: &Point) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut quad = Self::uninitialized();
            ffi::graphene_quad_init(
                quad.to_glib_none_mut().0,
                p1.to_glib_none().0,
                p2.to_glib_none().0,
                p3.to_glib_none().0,
                p4.to_glib_none().0,
            );
            quad
        }
    }

    #[doc(alias = "graphene_quad_init_from_rect")]
    #[doc(alias = "init_from_rect")]
    pub fn from_rect(r: &Rect) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut quad = Self::uninitialized();
            ffi::graphene_quad_init_from_rect(quad.to_glib_none_mut().0, r.to_glib_none().0);
            quad
        }
    }

    #[doc(alias = "graphene_quad_init_from_points")]
    #[doc(alias = "init_from_points")]
    pub fn from_points(points: &[Point; 4]) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let points = [
                *points[0].to_glib_none().0,
                *points[1].to_glib_none().0,
                *points[2].to_glib_none().0,
                *points[3].to_glib_none().0,
            ];
            let mut quad = Self::uninitialized();
            ffi::graphene_quad_init_from_points(
                quad.to_glib_none_mut().0,
                points.as_ptr() as *const _,
            );
            quad
        }
    }

    #[doc(alias = "graphene_quad_get_point")]
    #[doc(alias = "get_point")]
    pub fn point(&self, index_: u32) -> Point {
        assert!(index_ < 4);
        unsafe { from_glib_none(ffi::graphene_quad_get_point(self.to_glib_none().0, index_)) }
    }

    #[inline]
    pub fn points(&self) -> &[Point; 4] {
        unsafe { &*(&self.inner.points as *const [ffi::graphene_point_t; 4] as *const [Point; 4]) }
    }
}

impl fmt::Debug for Quad {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Quad")
            .field("points", &self.points())
            .finish()
    }
}
