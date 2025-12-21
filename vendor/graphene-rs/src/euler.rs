// Take a look at the license at the top of the repository in the LICENSE file.

use std::fmt;

use glib::translate::*;

use crate::{ffi, Euler, EulerOrder, Matrix, Quaternion, Vec3};

impl Euler {
    #[doc(alias = "graphene_euler_init")]
    pub fn new(x: f32, y: f32, z: f32) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut eul = Self::uninitialized();
            ffi::graphene_euler_init(eul.to_glib_none_mut().0, x, y, z);
            eul
        }
    }

    #[doc(alias = "graphene_euler_init_from_matrix")]
    #[doc(alias = "init_from_matrix")]
    pub fn from_matrix(m: Option<&Matrix>, order: EulerOrder) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut eul = Self::uninitialized();
            ffi::graphene_euler_init_from_matrix(
                eul.to_glib_none_mut().0,
                m.to_glib_none().0,
                order.into_glib(),
            );
            eul
        }
    }

    #[doc(alias = "graphene_euler_init_from_quaternion")]
    #[doc(alias = "init_from_quaternion")]
    pub fn from_quaternion(q: Option<&Quaternion>, order: EulerOrder) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut eul = Self::uninitialized();
            ffi::graphene_euler_init_from_quaternion(
                eul.to_glib_none_mut().0,
                q.to_glib_none().0,
                order.into_glib(),
            );
            eul
        }
    }

    #[doc(alias = "graphene_euler_init_from_radians")]
    #[doc(alias = "init_from_radians")]
    pub fn from_radians(x: f32, y: f32, z: f32, order: EulerOrder) -> Self {
        unsafe {
            let mut eul = Self::uninitialized();
            ffi::graphene_euler_init_from_radians(
                eul.to_glib_none_mut().0,
                x,
                y,
                z,
                order.into_glib(),
            );
            eul
        }
    }

    #[doc(alias = "graphene_euler_init_from_vec3")]
    #[doc(alias = "init_from_vec3")]
    pub fn from_vec3(v: Option<&Vec3>, order: EulerOrder) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut eul = Self::uninitialized();
            ffi::graphene_euler_init_from_vec3(
                eul.to_glib_none_mut().0,
                v.to_glib_none().0,
                order.into_glib(),
            );
            eul
        }
    }

    #[doc(alias = "graphene_euler_init_with_order")]
    #[doc(alias = "init_with_order")]
    pub fn with_order(x: f32, y: f32, z: f32, order: EulerOrder) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut eul = Self::uninitialized();
            ffi::graphene_euler_init_with_order(
                eul.to_glib_none_mut().0,
                x,
                y,
                z,
                order.into_glib(),
            );
            eul
        }
    }
}

impl fmt::Debug for Euler {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Euler")
            .field("order", &self.order())
            .field("alpha", &self.alpha())
            .field("beta", &self.beta())
            .field("gamma", &self.gamma())
            .finish()
    }
}
