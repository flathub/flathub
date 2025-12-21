// Take a look at the license at the top of the repository in the LICENSE file.

use std::fmt;

use glib::translate::*;

use crate::{ffi, Box, Point3D, Vec3};

impl Box {
    #[doc(alias = "graphene_box_get_vertices")]
    #[doc(alias = "get_vertices")]
    pub fn vertices(&self) -> &[Vec3; 8] {
        unsafe {
            let mut out: [ffi::graphene_vec3_t; 8] = std::mem::zeroed();
            ffi::graphene_box_get_vertices(self.to_glib_none().0, &mut out as *mut _);
            &*(&out as *const [ffi::graphene_vec3_t; 8] as *const [Vec3; 8])
        }
    }

    #[doc(alias = "graphene_box_init")]
    pub fn new(min: Option<&Point3D>, max: Option<&Point3D>) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut b = Self::uninitialized();
            ffi::graphene_box_init(
                b.to_glib_none_mut().0,
                min.to_glib_none().0,
                max.to_glib_none().0,
            );
            b
        }
    }

    #[doc(alias = "graphene_box_init_from_points")]
    #[doc(alias = "init_from_points")]
    pub fn from_points(points: &[Point3D]) -> Self {
        assert_initialized_main_thread!();

        let n = points.len() as u32;

        unsafe {
            let mut b = Self::uninitialized();
            ffi::graphene_box_init_from_points(b.to_glib_none_mut().0, n, points.to_glib_none().0);
            b
        }
    }

    #[doc(alias = "graphene_box_init_from_vec3")]
    #[doc(alias = "init_from_vec3")]
    pub fn from_vec3(min: Option<&Vec3>, max: Option<&Vec3>) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut b = Self::uninitialized();
            ffi::graphene_box_init_from_vec3(
                b.to_glib_none_mut().0,
                min.to_glib_none().0,
                max.to_glib_none().0,
            );
            b
        }
    }

    #[doc(alias = "graphene_box_init_from_vectors")]
    #[doc(alias = "init_from_vectors")]
    pub fn from_vectors(vectors: &[Vec3]) -> Self {
        assert_initialized_main_thread!();

        let n = vectors.len() as u32;

        unsafe {
            let mut b = Self::uninitialized();
            ffi::graphene_box_init_from_vectors(
                b.to_glib_none_mut().0,
                n,
                vectors.to_glib_none().0,
            );
            b
        }
    }
}

impl Default for Box {
    fn default() -> Self {
        Self::zero()
    }
}

impl fmt::Debug for Box {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Box")
            .field("min", &self.min())
            .field("max", &self.max())
            .finish()
    }
}
