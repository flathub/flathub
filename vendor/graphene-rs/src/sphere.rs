// Take a look at the license at the top of the repository in the LICENSE file.

use std::fmt;

use glib::translate::*;

use crate::{ffi, Point3D, Sphere, Vec3};

impl Sphere {
    #[doc(alias = "graphene_sphere_init")]
    pub fn new(center: Option<&Point3D>, radius: f32) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut sph = Self::uninitialized();
            ffi::graphene_sphere_init(sph.to_glib_none_mut().0, center.to_glib_none().0, radius);
            sph
        }
    }

    #[doc(alias = "graphene_sphere_init_from_points")]
    #[doc(alias = "init_from_points")]
    pub fn from_points(points: &[Point3D], center: Option<&Point3D>) -> Self {
        assert_initialized_main_thread!();

        let n = points.len() as u32;

        unsafe {
            let mut sph = Self::uninitialized();
            ffi::graphene_sphere_init_from_points(
                sph.to_glib_none_mut().0,
                n,
                points.to_glib_none().0,
                center.to_glib_none().0,
            );
            sph
        }
    }

    #[doc(alias = "graphene_sphere_init_from_vectors")]
    #[doc(alias = "init_from_vectors")]
    pub fn from_vectors(vectors: &[Vec3], center: Option<&Point3D>) -> Self {
        assert_initialized_main_thread!();

        let n = vectors.len() as u32;

        unsafe {
            let mut sph = Self::uninitialized();
            ffi::graphene_sphere_init_from_vectors(
                sph.to_glib_none_mut().0,
                n,
                vectors.to_glib_none().0,
                center.to_glib_none().0,
            );
            sph
        }
    }
}

impl fmt::Debug for Sphere {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Sphere")
            .field("radius", &self.radius())
            .field("center", &self.center())
            .finish()
    }
}
