// Take a look at the license at the top of the repository in the LICENSE file.

use std::fmt;

use glib::translate::*;

use crate::{ffi, Point3D, Triangle, Vec3};

impl Triangle {
    #[doc(alias = "graphene_triangle_init_from_float")]
    #[doc(alias = "init_from_float")]
    pub fn from_float(a: [f32; 3], b: [f32; 3], c: [f32; 3]) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut tri = Self::uninitialized();
            ffi::graphene_triangle_init_from_float(
                tri.to_glib_none_mut().0,
                a.as_ptr() as *const _,
                b.as_ptr() as *const _,
                c.as_ptr() as *const _,
            );
            tri
        }
    }

    #[doc(alias = "graphene_triangle_init_from_point3d")]
    #[doc(alias = "init_from_point3d")]
    pub fn from_point3d(a: Option<&Point3D>, b: Option<&Point3D>, c: Option<&Point3D>) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut tri = Self::uninitialized();
            ffi::graphene_triangle_init_from_point3d(
                tri.to_glib_none_mut().0,
                a.to_glib_none().0,
                b.to_glib_none().0,
                c.to_glib_none().0,
            );
            tri
        }
    }

    #[doc(alias = "graphene_triangle_init_from_vec3")]
    #[doc(alias = "init_from_vec3")]
    pub fn from_vec3(a: Option<&Vec3>, b: Option<&Vec3>, c: Option<&Vec3>) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut tri = Self::uninitialized();
            ffi::graphene_triangle_init_from_vec3(
                tri.to_glib_none_mut().0,
                a.to_glib_none().0,
                b.to_glib_none().0,
                c.to_glib_none().0,
            );
            tri
        }
    }
}

impl fmt::Debug for Triangle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Triangle")
            .field("points", &self.points())
            .finish()
    }
}
