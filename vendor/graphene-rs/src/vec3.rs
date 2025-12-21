// Take a look at the license at the top of the repository in the LICENSE file.

use std::{fmt, ops};

use glib::translate::*;

use crate::{ffi, Vec3};

impl Vec3 {
    #[doc(alias = "graphene_vec3_init")]
    pub fn new(x: f32, y: f32, z: f32) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut vec = Self::uninitialized();
            ffi::graphene_vec3_init(vec.to_glib_none_mut().0, x, y, z);
            vec
        }
    }

    #[doc(alias = "graphene_vec3_init_from_float")]
    #[doc(alias = "init_from_float")]
    pub fn from_float(src: [f32; 3]) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut vec = Self::uninitialized();
            ffi::graphene_vec3_init_from_float(vec.to_glib_none_mut().0, src.as_ptr() as *const _);
            vec
        }
    }

    #[doc(alias = "graphene_vec3_to_float")]
    pub fn to_float(&self) -> [f32; 3] {
        unsafe {
            let mut out = std::mem::MaybeUninit::uninit();
            ffi::graphene_vec3_to_float(self.to_glib_none().0, out.as_mut_ptr());
            out.assume_init()
        }
    }
}

impl fmt::Debug for Vec3 {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Vec3")
            .field("x", &self.x())
            .field("y", &self.y())
            .field("z", &self.z())
            .finish()
    }
}

impl Default for Vec3 {
    fn default() -> Self {
        Self::zero()
    }
}

// addition/subtraction
impl ops::Add<Vec3> for Vec3 {
    type Output = Vec3;

    fn add(self, rhs: Vec3) -> Self::Output {
        Vec3::add(&self, &rhs)
    }
}
impl ops::AddAssign<Vec3> for Vec3 {
    fn add_assign(&mut self, rhs: Vec3) {
        *self = *self + rhs;
    }
}
impl ops::Sub<Vec3> for Vec3 {
    type Output = Vec3;

    fn sub(self, rhs: Vec3) -> Self::Output {
        Vec3::subtract(&self, &rhs)
    }
}
impl ops::SubAssign<Vec3> for Vec3 {
    fn sub_assign(&mut self, rhs: Vec3) {
        *self = *self - rhs;
    }
}
impl ops::Neg for Vec3 {
    type Output = Vec3;

    fn neg(self) -> Self::Output {
        Vec3::negate(&self)
    }
}

// scalar multiplication
impl ops::Mul<f32> for Vec3 {
    type Output = Vec3;

    fn mul(self, rhs: f32) -> Self::Output {
        Vec3::scale(&self, rhs)
    }
}
impl ops::MulAssign<f32> for Vec3 {
    fn mul_assign(&mut self, rhs: f32) {
        *self = *self * rhs;
    }
}
impl ops::Mul<Vec3> for f32 {
    type Output = Vec3;

    fn mul(self, rhs: Vec3) -> Self::Output {
        rhs * self
    }
}

// Component-wise multiplication/division
impl ops::Mul<Vec3> for Vec3 {
    type Output = Vec3;

    fn mul(self, rhs: Vec3) -> Self::Output {
        Vec3::multiply(&self, &rhs)
    }
}
impl ops::MulAssign<Vec3> for Vec3 {
    fn mul_assign(&mut self, rhs: Vec3) {
        *self = *self * rhs;
    }
}
impl ops::Div<Vec3> for Vec3 {
    type Output = Vec3;

    fn div(self, rhs: Vec3) -> Self::Output {
        Vec3::divide(&self, &rhs)
    }
}
impl ops::DivAssign<Vec3> for Vec3 {
    fn div_assign(&mut self, rhs: Vec3) {
        *self = *self / rhs;
    }
}
