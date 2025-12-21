// Take a look at the license at the top of the repository in the LICENSE file.

use std::fmt;

use glib::translate::*;

use crate::{ffi, Frustum, Matrix, Plane};

impl Frustum {
    #[doc(alias = "graphene_frustum_get_planes")]
    #[doc(alias = "get_planes")]
    pub fn planes(&self) -> &[Plane; 6] {
        unsafe {
            let mut out: [ffi::graphene_plane_t; 6] = std::mem::zeroed();
            ffi::graphene_frustum_get_planes(self.to_glib_none().0, &mut out as *mut _);
            &*(&out as *const [ffi::graphene_plane_t; 6] as *const [Plane; 6])
        }
    }

    #[doc(alias = "graphene_frustum_init")]
    pub fn new(p0: &Plane, p1: &Plane, p2: &Plane, p3: &Plane, p4: &Plane, p5: &Plane) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut fru = Self::uninitialized();
            ffi::graphene_frustum_init(
                fru.to_glib_none_mut().0,
                p0.to_glib_none().0,
                p1.to_glib_none().0,
                p2.to_glib_none().0,
                p3.to_glib_none().0,
                p4.to_glib_none().0,
                p5.to_glib_none().0,
            );
            fru
        }
    }

    #[doc(alias = "graphene_frustum_init_from_matrix")]
    #[doc(alias = "init_from_matrix")]
    pub fn from_matrix(matrix: &Matrix) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut fru = Self::uninitialized();
            ffi::graphene_frustum_init_from_matrix(
                fru.to_glib_none_mut().0,
                matrix.to_glib_none().0,
            );
            fru
        }
    }
}

impl fmt::Debug for Frustum {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Frustum")
            .field("planes", &self.planes())
            .finish()
    }
}
