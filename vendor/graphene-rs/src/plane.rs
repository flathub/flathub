// Take a look at the license at the top of the repository in the LICENSE file.

use std::fmt;

use glib::translate::*;

use crate::{ffi, Plane, Point3D, Vec3, Vec4};

impl Plane {
    #[doc(alias = "graphene_plane_init")]
    pub fn new(normal: Option<&Vec3>, constant: f32) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut plane = Self::uninitialized();
            ffi::graphene_plane_init(
                plane.to_glib_none_mut().0,
                normal.to_glib_none().0,
                constant,
            );
            plane
        }
    }

    #[doc(alias = "graphene_plane_init_from_point")]
    #[doc(alias = "init_from_point")]
    pub fn from_point(normal: &Vec3, point: &Point3D) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut plane = Self::uninitialized();
            ffi::graphene_plane_init_from_point(
                plane.to_glib_none_mut().0,
                normal.to_glib_none().0,
                point.to_glib_none().0,
            );
            plane
        }
    }

    #[doc(alias = "graphene_plane_init_from_points")]
    #[doc(alias = "init_from_points")]
    pub fn from_points(a: &Point3D, b: &Point3D, c: &Point3D) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut plane = Self::uninitialized();
            ffi::graphene_plane_init_from_points(
                plane.to_glib_none_mut().0,
                a.to_glib_none().0,
                b.to_glib_none().0,
                c.to_glib_none().0,
            );
            plane
        }
    }

    #[doc(alias = "graphene_plane_init_from_vec4")]
    #[doc(alias = "init_from_vec4")]
    pub fn from_vec4(src: &Vec4) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut plane = Self::uninitialized();
            ffi::graphene_plane_init_from_vec4(plane.to_glib_none_mut().0, src.to_glib_none().0);
            plane
        }
    }
}

impl fmt::Debug for Plane {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Plane")
            .field("constant", &self.constant())
            .field("normal", &self.normal())
            .finish()
    }
}
