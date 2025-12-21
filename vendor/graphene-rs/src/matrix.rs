// Take a look at the license at the top of the repository in the LICENSE file.

use std::{fmt, ops};

use glib::translate::*;

use crate::{ffi, Matrix, Point, Point3D, Vec3, Vec4};

impl Matrix {
    #[doc(alias = "graphene_matrix_init_from_2d")]
    #[doc(alias = "init_from_2d")]
    pub fn from_2d(xx: f64, yx: f64, xy: f64, yy: f64, x_0: f64, y_0: f64) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut mat = Self::uninitialized();
            ffi::graphene_matrix_init_from_2d(mat.to_glib_none_mut().0, xx, yx, xy, yy, x_0, y_0);
            mat
        }
    }

    #[doc(alias = "graphene_matrix_init_from_float")]
    #[doc(alias = "init_from_float")]
    pub fn from_float(v: [f32; 16]) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut mat = Self::uninitialized();
            ffi::graphene_matrix_init_from_float(mat.to_glib_none_mut().0, v.as_ptr() as *const _);
            mat
        }
    }

    #[doc(alias = "graphene_matrix_init_from_vec4")]
    #[doc(alias = "init_from_vec4")]
    pub fn from_vec4(v0: &Vec4, v1: &Vec4, v2: &Vec4, v3: &Vec4) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut mat = Self::uninitialized();
            ffi::graphene_matrix_init_from_vec4(
                mat.to_glib_none_mut().0,
                v0.to_glib_none().0,
                v1.to_glib_none().0,
                v2.to_glib_none().0,
                v3.to_glib_none().0,
            );
            mat
        }
    }

    #[doc(alias = "graphene_matrix_init_frustum")]
    #[doc(alias = "init_frustum")]
    pub fn new_frustum(
        left: f32,
        right: f32,
        bottom: f32,
        top: f32,
        z_near: f32,
        z_far: f32,
    ) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut mat = Self::uninitialized();
            ffi::graphene_matrix_init_frustum(
                mat.to_glib_none_mut().0,
                left,
                right,
                bottom,
                top,
                z_near,
                z_far,
            );
            mat
        }
    }

    #[doc(alias = "graphene_matrix_init_identity")]
    #[doc(alias = "init_identity")]
    pub fn new_identity() -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut mat = Self::uninitialized();
            ffi::graphene_matrix_init_identity(mat.to_glib_none_mut().0);
            mat
        }
    }

    #[doc(alias = "graphene_matrix_init_look_at")]
    #[doc(alias = "init_look_at")]
    pub fn new_look_at(eye: &Vec3, center: &Vec3, up: &Vec3) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut mat = Self::uninitialized();
            ffi::graphene_matrix_init_look_at(
                mat.to_glib_none_mut().0,
                eye.to_glib_none().0,
                center.to_glib_none().0,
                up.to_glib_none().0,
            );
            mat
        }
    }

    #[doc(alias = "graphene_matrix_init_ortho")]
    #[doc(alias = "init_ortho")]
    pub fn new_ortho(
        left: f32,
        right: f32,
        top: f32,
        bottom: f32,
        z_near: f32,
        z_far: f32,
    ) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut mat = Self::uninitialized();
            ffi::graphene_matrix_init_ortho(
                mat.to_glib_none_mut().0,
                left,
                right,
                top,
                bottom,
                z_near,
                z_far,
            );
            mat
        }
    }

    #[doc(alias = "graphene_matrix_init_perspective")]
    #[doc(alias = "init_perspective")]
    pub fn new_perspective(fovy: f32, aspect: f32, z_near: f32, z_far: f32) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut mat = Self::uninitialized();
            ffi::graphene_matrix_init_perspective(
                mat.to_glib_none_mut().0,
                fovy,
                aspect,
                z_near,
                z_far,
            );
            mat
        }
    }

    #[doc(alias = "graphene_matrix_init_rotate")]
    #[doc(alias = "init_rotate")]
    pub fn new_rotate(angle: f32, axis: &Vec3) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut mat = Self::uninitialized();
            ffi::graphene_matrix_init_rotate(
                mat.to_glib_none_mut().0,
                angle,
                axis.to_glib_none().0,
            );
            mat
        }
    }

    #[doc(alias = "graphene_matrix_init_scale")]
    #[doc(alias = "init_scale")]
    pub fn new_scale(x: f32, y: f32, z: f32) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut mat = Self::uninitialized();
            ffi::graphene_matrix_init_scale(mat.to_glib_none_mut().0, x, y, z);
            mat
        }
    }

    #[doc(alias = "graphene_matrix_init_skew")]
    #[doc(alias = "init_skew")]
    pub fn new_skew(x_skew: f32, y_skew: f32) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut mat = Self::uninitialized();
            ffi::graphene_matrix_init_skew(mat.to_glib_none_mut().0, x_skew, y_skew);
            mat
        }
    }

    #[doc(alias = "graphene_matrix_init_translate")]
    #[doc(alias = "init_translate")]
    pub fn new_translate(p: &Point3D) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut mat = Self::uninitialized();
            ffi::graphene_matrix_init_translate(mat.to_glib_none_mut().0, p.to_glib_none().0);
            mat
        }
    }

    #[doc(alias = "graphene_matrix_to_float")]
    pub fn to_float(&self) -> [f32; 16] {
        unsafe {
            let mut out = std::mem::MaybeUninit::uninit();
            ffi::graphene_matrix_to_float(self.to_glib_none().0, out.as_mut_ptr());
            out.assume_init()
        }
    }

    #[inline]
    pub fn values(&self) -> &[[f32; 4]; 4] {
        unsafe { &*(&self.inner.value as *const ffi::graphene_simd4x4f_t as *const [[f32; 4]; 4]) }
    }
}

impl fmt::Debug for Matrix {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Matrix")
            .field("values", &self.values())
            .finish()
    }
}

impl Default for Matrix {
    fn default() -> Self {
        Self::new_identity()
    }
}

// Scalar multiplication
impl ops::Mul<Matrix> for f32 {
    type Output = Matrix;

    fn mul(self, mut rhs: Matrix) -> Self::Output {
        rhs.scale(self, self, self);
        rhs
    }
}

// Matrix-matrix/-vector multiplication
impl ops::Mul<Matrix> for Matrix {
    type Output = Matrix;

    fn mul(self, rhs: Matrix) -> Self::Output {
        Matrix::multiply(&self, &rhs)
    }
}
impl ops::MulAssign<Matrix> for Matrix {
    fn mul_assign(&mut self, rhs: Matrix) {
        *self = *self * rhs;
    }
}

impl ops::Mul<Vec4> for Matrix {
    type Output = Vec4;

    // rustdoc-stripper-ignore-next
    /// Transforms this `Vec4` using the provided matrix.
    /// See [Matrix::transform_vec4].
    fn mul(self, rhs: Vec4) -> Self::Output {
        Matrix::transform_vec4(&self, &rhs)
    }
}

impl ops::Mul<Vec3> for Matrix {
    type Output = Vec3;

    // rustdoc-stripper-ignore-next
    /// Transforms this `Vec3` using the provided matrix.
    /// See [Matrix::transform_vec3].
    fn mul(self, rhs: Vec3) -> Self::Output {
        Matrix::transform_vec3(&self, &rhs)
    }
}

impl ops::Mul<Point> for Matrix {
    type Output = Point;

    fn mul(self, rhs: Point) -> Self::Output {
        Matrix::transform_point(&self, &rhs)
    }
}

impl ops::Mul<Point3D> for Matrix {
    type Output = Point3D;

    // rustdoc-stripper-ignore-next
    /// Transforms this point using the provided matrix.
    /// See [Matrix::transform_point3d].
    fn mul(self, rhs: Point3D) -> Self::Output {
        Matrix::transform_point3d(&self, &rhs)
    }
}

#[cfg(test)]
mod tests {
    use super::Matrix;
    #[test]
    fn test_matrix_values() {
        let matrix = Matrix::new_identity();
        assert_eq!(
            matrix.values(),
            &[
                [1.0, 0.0, 0.0, 0.0],
                [0.0, 1.0, 0.0, 0.0],
                [0.0, 0.0, 1.0, 0.0],
                [0.0, 0.0, 0.0, 1.0]
            ],
        );
    }
}
