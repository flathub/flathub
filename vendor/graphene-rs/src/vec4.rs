// Take a look at the license at the top of the repository in the LICENSE file.

use std::{fmt, ops};

use glib::translate::*;

use crate::{ffi, Vec2, Vec3, Vec4};

impl Vec4 {
    #[doc(alias = "graphene_vec4_init")]
    pub fn new(x: f32, y: f32, z: f32, w: f32) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut vec = Self::uninitialized();
            ffi::graphene_vec4_init(vec.to_glib_none_mut().0, x, y, z, w);
            vec
        }
    }

    #[doc(alias = "graphene_vec4_init_from_vec2")]
    #[doc(alias = "init_from_vec2")]
    pub fn from_vec2(src: &Vec2, z: f32, w: f32) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut vec = Self::uninitialized();
            ffi::graphene_vec4_init_from_vec2(vec.to_glib_none_mut().0, src.to_glib_none().0, z, w);
            vec
        }
    }

    #[doc(alias = "graphene_vec4_init_from_vec3")]
    #[doc(alias = "init_from_vec3")]
    pub fn from_vec3(src: &Vec3, w: f32) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut vec = Self::uninitialized();
            ffi::graphene_vec4_init_from_vec3(vec.to_glib_none_mut().0, src.to_glib_none().0, w);
            vec
        }
    }

    #[doc(alias = "graphene_vec4_init_from_float")]
    #[doc(alias = "init_from_float")]
    pub fn from_float(src: [f32; 4]) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut vec = Self::uninitialized();
            ffi::graphene_vec4_init_from_float(vec.to_glib_none_mut().0, src.as_ptr() as *const _);
            vec
        }
    }

    #[doc(alias = "graphene_vec4_to_float")]
    pub fn to_float(&self) -> [f32; 4] {
        unsafe {
            let mut out = std::mem::MaybeUninit::uninit();
            ffi::graphene_vec4_to_float(self.to_glib_none().0, out.as_mut_ptr());
            out.assume_init()
        }
    }
}

impl fmt::Debug for Vec4 {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Vec4")
            .field("x", &self.x())
            .field("y", &self.y())
            .field("z", &self.z())
            .field("w", &self.w())
            .finish()
    }
}

impl Default for Vec4 {
    fn default() -> Self {
        Self::zero()
    }
}

// addition/subtraction
impl ops::Add<Vec4> for Vec4 {
    type Output = Vec4;

    fn add(self, rhs: Vec4) -> Self::Output {
        Vec4::add(&self, &rhs)
    }
}
impl ops::AddAssign<Vec4> for Vec4 {
    fn add_assign(&mut self, rhs: Vec4) {
        *self = *self + rhs;
    }
}
impl ops::Sub<Vec4> for Vec4 {
    type Output = Vec4;

    fn sub(self, rhs: Vec4) -> Self::Output {
        Vec4::subtract(&self, &rhs)
    }
}
impl ops::SubAssign<Vec4> for Vec4 {
    fn sub_assign(&mut self, rhs: Vec4) {
        *self = *self - rhs;
    }
}
impl ops::Neg for Vec4 {
    type Output = Vec4;

    fn neg(self) -> Self::Output {
        Vec4::negate(&self)
    }
}

// scalar multiplication
impl ops::Mul<f32> for Vec4 {
    type Output = Vec4;

    fn mul(self, rhs: f32) -> Self::Output {
        Vec4::scale(&self, rhs)
    }
}
impl ops::MulAssign<f32> for Vec4 {
    fn mul_assign(&mut self, rhs: f32) {
        *self = *self * rhs;
    }
}
impl ops::Mul<Vec4> for f32 {
    type Output = Vec4;

    fn mul(self, rhs: Vec4) -> Self::Output {
        rhs * self
    }
}

// Component-wise multiplication/division
impl ops::Mul<Vec4> for Vec4 {
    type Output = Vec4;

    fn mul(self, rhs: Vec4) -> Self::Output {
        Vec4::multiply(&self, &rhs)
    }
}
impl ops::MulAssign<Vec4> for Vec4 {
    fn mul_assign(&mut self, rhs: Vec4) {
        *self = *self * rhs;
    }
}
impl ops::Div<Vec4> for Vec4 {
    type Output = Vec4;

    fn div(self, rhs: Vec4) -> Self::Output {
        Vec4::divide(&self, &rhs)
    }
}
impl ops::DivAssign<Vec4> for Vec4 {
    fn div_assign(&mut self, rhs: Vec4) {
        *self = *self / rhs;
    }
}
