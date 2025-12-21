use crate::{prelude::*, NavigationDirection, Swipeable};
use glib::translate::*;
use gtk::subclass::prelude::*;

pub trait SwipeableImpl: WidgetImpl + ObjectSubclass<Type: IsA<Swipeable>> {
    fn cancel_progress(&self) -> f64 {
        self.parent_cancel_progress()
    }

    fn distance(&self) -> f64 {
        self.parent_distance()
    }

    fn progress(&self) -> f64 {
        self.parent_progress()
    }

    fn snap_points(&self) -> Vec<f64> {
        self.parent_snap_points()
    }

    fn swipe_area(
        &self,
        navigation_direction: NavigationDirection,
        is_drag: bool,
    ) -> gdk::Rectangle {
        self.parent_swipe_area(navigation_direction, is_drag)
    }
}
pub trait SwipeableImplExt: SwipeableImpl {
    fn parent_cancel_progress(&self) -> f64 {
        unsafe {
            let type_data = Self::type_data();
            let parent_iface = type_data.as_ref().parent_interface::<Swipeable>()
                as *const ffi::AdwSwipeableInterface;

            let func = (*parent_iface)
                .get_cancel_progress
                .expect("no parent \"get_cancel_progress\" implementation");

            func(self.obj().unsafe_cast_ref::<Swipeable>().to_glib_none().0)
        }
    }

    fn parent_distance(&self) -> f64 {
        unsafe {
            let type_data = Self::type_data();
            let parent_iface = type_data.as_ref().parent_interface::<Swipeable>()
                as *const ffi::AdwSwipeableInterface;

            let func = (*parent_iface)
                .get_distance
                .expect("no parent \"get_distance\" implementation");

            func(self.obj().unsafe_cast_ref::<Swipeable>().to_glib_none().0)
        }
    }

    fn parent_progress(&self) -> f64 {
        unsafe {
            let type_data = Self::type_data();
            let parent_iface = type_data.as_ref().parent_interface::<Swipeable>()
                as *const ffi::AdwSwipeableInterface;

            let func = (*parent_iface)
                .get_progress
                .expect("no parent \"get_progress\" implementation");

            func(self.obj().unsafe_cast_ref::<Swipeable>().to_glib_none().0)
        }
    }

    fn parent_snap_points(&self) -> Vec<f64> {
        unsafe {
            let type_data = Self::type_data();
            let parent_iface = type_data.as_ref().parent_interface::<Swipeable>()
                as *const ffi::AdwSwipeableInterface;

            let func = (*parent_iface)
                .get_snap_points
                .expect("no parent \"get_snap_points\" implementation");

            let mut n_points = std::mem::MaybeUninit::uninit();

            let points = func(
                self.obj().unsafe_cast_ref::<Swipeable>().to_glib_none().0,
                n_points.as_mut_ptr(),
            );

            let size = n_points.assume_init() as usize;
            Vec::from_raw_parts(points, size, size)
        }
    }

    fn parent_swipe_area(
        &self,
        navigation_direction: NavigationDirection,
        is_drag: bool,
    ) -> gdk::Rectangle {
        unsafe {
            let type_data = Self::type_data();
            let parent_iface = type_data.as_ref().parent_interface::<Swipeable>()
                as *const ffi::AdwSwipeableInterface;

            let func = (*parent_iface)
                .get_swipe_area
                .expect("no parent \"get_swipe_area\" implementation");

            let mut rect = gdk::Rectangle::uninitialized();
            func(
                self.obj().unsafe_cast_ref::<Swipeable>().to_glib_none().0,
                navigation_direction.into_glib(),
                is_drag.into_glib(),
                rect.to_glib_none_mut().0,
            );

            rect
        }
    }
}

impl<T: SwipeableImpl> SwipeableImplExt for T {}

unsafe impl<T: SwipeableImpl> IsImplementable<T> for Swipeable {
    fn interface_init(iface: &mut glib::Interface<Self>) {
        let iface = iface.as_mut();

        iface.get_cancel_progress = Some(swipeable_get_cancel_progress::<T>);
        iface.get_distance = Some(swipeable_get_distance::<T>);
        iface.get_progress = Some(swipeable_get_progress::<T>);
        iface.get_snap_points = Some(swipeable_get_snap_points::<T>);
        iface.get_swipe_area = Some(swipeable_get_swipe_area::<T>);
    }
}

unsafe extern "C" fn swipeable_get_cancel_progress<T: SwipeableImpl>(
    swipeable: *mut ffi::AdwSwipeable,
) -> f64 {
    let instance = unsafe { &*(swipeable as *mut T::Instance) };
    let imp = instance.imp();

    imp.cancel_progress()
}

unsafe extern "C" fn swipeable_get_distance<T: SwipeableImpl>(
    swipeable: *mut ffi::AdwSwipeable,
) -> f64 {
    let instance = unsafe { &*(swipeable as *mut T::Instance) };
    let imp = instance.imp();

    imp.distance()
}

unsafe extern "C" fn swipeable_get_progress<T: SwipeableImpl>(
    swipeable: *mut ffi::AdwSwipeable,
) -> f64 {
    let instance = unsafe { &*(swipeable as *mut T::Instance) };
    let imp = instance.imp();

    imp.progress()
}

unsafe extern "C" fn swipeable_get_snap_points<T: SwipeableImpl>(
    swipeable: *mut ffi::AdwSwipeable,
    n_pointsptr: *mut libc::c_int,
) -> *mut f64 {
    let instance = unsafe { &*(swipeable as *mut T::Instance) };
    let imp = instance.imp();

    let points = imp.snap_points();

    unsafe {
        n_pointsptr.write(points.len() as libc::c_int);
    }
    ToGlibContainerFromSlice::to_glib_full_from_slice(points.as_slice())
}

unsafe extern "C" fn swipeable_get_swipe_area<T: SwipeableImpl>(
    swipeable: *mut ffi::AdwSwipeable,
    navigation_direction: ffi::AdwNavigationDirection,
    is_drag: i32,
    area: *mut gdk::ffi::GdkRectangle,
) {
    unsafe {
        let instance = &*(swipeable as *mut T::Instance);
        let imp = instance.imp();

        let swipe_area = imp.swipe_area(from_glib(navigation_direction), from_glib(is_drag));

        *area = *swipe_area.to_glib_full();
    }
}
