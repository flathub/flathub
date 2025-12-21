// Take a look at the license at the top of the repository in the LICENSE file.

use std::{fmt, ops};

use glib::translate::*;

use crate::{ffi, Vec2};

impl Vec2 {
    #[doc(alias = "graphene_vec2_init")]
    pub fn new(x: f32, y: f32) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut vec = Self::uninitialized();
            ffi::graphene_vec2_init(vec.to_glib_none_mut().0, x, y);
            vec
        }
    }

    #[doc(alias = "graphene_vec2_init_from_float")]
    #[doc(alias = "init_from_float")]
    pub fn from_float(src: [f32; 2]) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut vec = Self::uninitialized();
            ffi::graphene_vec2_init_from_float(vec.to_glib_none_mut().0, src.as_ptr() as *const _);
            vec
        }
    }

    #[doc(alias = "graphene_vec2_to_float")]
    pub fn to_float(&self) -> [f32; 2] {
        unsafe {
            let mut out = std::mem::MaybeUninit::uninit();
            ffi::graphene_vec2_to_float(self.to_glib_none().0, out.as_mut_ptr());
            out.assume_init()
        }
    }
}

impl fmt::Debug for Vec2 {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Vec2")
            .field("x", &self.x())
            .field("y", &self.y())
            .finish()
    }
}

impl Default for Vec2 {
    fn default() -> Self {
        Self::zero()
    }
}

// addition/subtraction
impl ops::Add<Vec2> for Vec2 {
    type Output = Vec2;

    fn add(self, rhs: Vec2) -> Self::Output {
        Vec2::add(&self, &rhs)
    }
}
impl ops::AddAssign<Vec2> for Vec2 {
    fn add_assign(&mut self, rhs: Vec2) {
        *self = *self + rhs;
    }
}
impl ops::Sub<Vec2> for Vec2 {
    type Output = Vec2;

    fn sub(self, rhs: Vec2) -> Self::Output {
        Vec2::subtract(&self, &rhs)
    }
}
impl ops::SubAssign<Vec2> for Vec2 {
    fn sub_assign(&mut self, rhs: Vec2) {
        *self = *self - rhs;
    }
}
impl ops::Neg for Vec2 {
    type Output = Vec2;

    fn neg(self) -> Self::Output {
        Vec2::negate(&self)
    }
}

// scalar multiplication
impl ops::Mul<f32> for Vec2 {
    type Output = Vec2;

    fn mul(self, rhs: f32) -> Self::Output {
        Vec2::scale(&self, rhs)
    }
}
impl ops::MulAssign<f32> for Vec2 {
    fn mul_assign(&mut self, rhs: f32) {
        *self = *self * rhs;
    }
}
impl ops::Mul<Vec2> for f32 {
    type Output = Vec2;

    fn mul(self, rhs: Vec2) -> Self::Output {
        rhs * self
    }
}

// Component-wise multiplication/division
impl ops::Mul<Vec2> for Vec2 {
    type Output = Vec2;

    fn mul(self, rhs: Vec2) -> Self::Output {
        Vec2::multiply(&self, &rhs)
    }
}
impl ops::MulAssign<Vec2> for Vec2 {
    fn mul_assign(&mut self, rhs: Vec2) {
        *self = *self * rhs;
    }
}
impl ops::Div<Vec2> for Vec2 {
    type Output = Vec2;

    fn div(self, rhs: Vec2) -> Self::Output {
        Vec2::divide(&self, &rhs)
    }
}
impl ops::DivAssign<Vec2> for Vec2 {
    fn div_assign(&mut self, rhs: Vec2) {
        *self = *self / rhs;
    }
}
