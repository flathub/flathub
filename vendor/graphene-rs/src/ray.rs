// Take a look at the license at the top of the repository in the LICENSE file.

use std::fmt;

use glib::translate::*;

use crate::{ffi, Box, Point3D, Ray, RayIntersectionKind, Sphere, Triangle, Vec3};

impl Ray {
    #[doc(alias = "graphene_ray_init")]
    pub fn new(origin: Option<&Point3D>, direction: Option<&Vec3>) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut ray = Self::uninitialized();
            ffi::graphene_ray_init(
                ray.to_glib_none_mut().0,
                origin.to_glib_none().0,
                direction.to_glib_none().0,
            );
            ray
        }
    }

    #[doc(alias = "graphene_ray_init_from_vec3")]
    #[doc(alias = "init_from_vec3")]
    pub fn from_vec3(origin: Option<&Vec3>, direction: Option<&Vec3>) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut ray = Self::uninitialized();
            ffi::graphene_ray_init_from_vec3(
                ray.to_glib_none_mut().0,
                origin.to_glib_none().0,
                direction.to_glib_none().0,
            );
            ray
        }
    }

    #[doc(alias = "graphene_ray_intersect_box")]
    pub fn intersect_box(&self, b: &Box) -> (RayIntersectionKind, Option<f32>) {
        unsafe {
            let mut t_out = std::mem::MaybeUninit::uninit();
            let ret = from_glib(ffi::graphene_ray_intersect_box(
                self.to_glib_none().0,
                b.to_glib_none().0,
                t_out.as_mut_ptr(),
            ));
            match ret {
                RayIntersectionKind::None => (ret, None),
                _ => (ret, Some(t_out.assume_init())),
            }
        }
    }

    #[doc(alias = "graphene_ray_intersect_sphere")]
    pub fn intersect_sphere(&self, s: &Sphere) -> (RayIntersectionKind, Option<f32>) {
        unsafe {
            let mut t_out = std::mem::MaybeUninit::uninit();
            let ret = from_glib(ffi::graphene_ray_intersect_sphere(
                self.to_glib_none().0,
                s.to_glib_none().0,
                t_out.as_mut_ptr(),
            ));
            match ret {
                RayIntersectionKind::None => (ret, None),
                _ => (ret, Some(t_out.assume_init())),
            }
        }
    }

    #[doc(alias = "graphene_ray_intersect_triangle")]
    pub fn intersect_triangle(&self, t: &Triangle) -> (RayIntersectionKind, Option<f32>) {
        unsafe {
            let mut t_out = std::mem::MaybeUninit::uninit();
            let ret = from_glib(ffi::graphene_ray_intersect_triangle(
                self.to_glib_none().0,
                t.to_glib_none().0,
                t_out.as_mut_ptr(),
            ));
            match ret {
                RayIntersectionKind::None => (ret, None),
                _ => (ret, Some(t_out.assume_init())),
            }
        }
    }
}

impl fmt::Debug for Ray {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Ray")
            .field("origin", &self.origin())
            .field("direction", &self.direction())
            .finish()
    }
}
