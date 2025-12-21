// Take a look at the license at the top of the repository in the LICENSE file.

use std::fmt;

use glib::translate::*;

use crate::{ffi, Rect, Vec2};

impl Rect {
    #[doc(alias = "graphene_rect_get_vertices")]
    #[doc(alias = "get_vertices")]
    pub fn vertices(&self) -> &[Vec2; 4] {
        unsafe {
            let mut out: [ffi::graphene_vec2_t; 4] = std::mem::zeroed();
            ffi::graphene_rect_get_vertices(self.to_glib_none().0, &mut out as *mut _);
            &*(&out as *const [ffi::graphene_vec2_t; 4] as *const [Vec2; 4])
        }
    }

    #[doc(alias = "graphene_rect_init")]
    pub fn new(x: f32, y: f32, width: f32, height: f32) -> Self {
        assert_initialized_main_thread!();
        unsafe {
            let mut rect = Self::uninitialized();
            ffi::graphene_rect_init(rect.to_glib_none_mut().0, x, y, width, height);
            rect
        }
    }
}

impl fmt::Debug for Rect {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Rect")
            .field("x", &self.x())
            .field("y", &self.y())
            .field("width", &self.width())
            .field("height", &self.height())
            .finish()
    }
}

impl Default for Rect {
    fn default() -> Self {
        Self::zero()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::Point;

    #[test]
    fn contains_point() {
        let rect = Rect::new(100., 100., 100., 100.);

        let right = Point::new(250., 150.);
        let below = Point::new(150., 50.);
        let left = Point::new(50., 150.);
        let above = Point::new(150., 250.);

        assert!(!rect.contains_point(&right));
        assert!(!rect.contains_point(&below));
        assert!(!rect.contains_point(&left));
        assert!(!rect.contains_point(&above));
    }
}
