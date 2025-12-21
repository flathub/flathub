// Take a look at the license at the top of the repository in the LICENSE file.

use glib::{prelude::*, subclass::prelude::*, translate::*};

use crate::{ffi, prelude::*, Cancellable, FileEnumerator, FileInfo, IOErrorEnum};

// Support custom implementation of virtual functions defined in `gio::ffi::GFileEnumeratorClass` except pairs `xxx_async/xxx_finish` for which GIO provides a default implementation (which should be ok).
// TODO: overriding these default implementations might still be useful for subclasses (if they can do something better than blocking IO).
pub trait FileEnumeratorImpl: ObjectImpl + ObjectSubclass<Type: IsA<FileEnumerator>> {
    fn next_file(
        &self,
        cancellable: Option<&Cancellable>,
    ) -> Result<Option<FileInfo>, glib::Error> {
        self.parent_next_file(cancellable)
    }

    // rustdoc-stripper-ignore-next
    /// Closes the enumerator (see [`FileEnumeratorExt::close`]).
    ///
    /// NOTE: If the enumerator has not been explicitly closed, GIO closes it when the object is dropped.
    /// But GIO does it by calling `close` vfunc in `finalize`, which is not safe and could lead to undefined behavior,
    /// such as accessing freed memory or resources, which can cause crashes or other unexpected behavior.
    ///
    /// An issue has been opened in GLib to address this: <https://gitlab.gnome.org/GNOME/glib/-/issues/3713> and a MR has been opened to fix it: <https://gitlab.gnome.org/GNOME/glib/-/merge_requests/4672>.
    ///
    /// Until this is fixed, it is unsafe to rely on the enumerator being closed when the object is dropped.
    /// It is recommended to close the enumerator explicitly before dropping it, by calling [`FileEnumeratorExt::close`],
    /// or to implement the [`ObjectImpl::dispose`] method and call [`FileEnumeratorExt::close`] there (it is safe to access the object there):
    /// ```ignore
    /// pub struct MyFileEnumerator();
    ///
    /// #[glib::object_subclass]
    /// impl ObjectSubclass for MyFileEnumerator { ... }
    ///
    /// impl ObjectImpl for MyFileEnumerator {
    ///     fn dispose(&self) {
    ///         // close the enumerator here is safe and avoids `finalize` to call close.
    ///         let _ = self.obj().close(Cancellable::NONE);
    ///     }
    /// }
    ///
    /// impl FileEnumeratorImpl for MyFileEnumerator { ... }
    /// ```
    ///
    /// [`FileEnumeratorExt::close`]: ../auto/file_enumerator/trait.FileEnumeratorExt.html#method.close
    /// [`ObjectImpl::dispose`]: ../../glib/subclass/object/trait.ObjectImpl.html#method.dispose
    fn close(&self, cancellable: Option<&Cancellable>) -> (bool, Option<glib::Error>) {
        self.parent_close(cancellable)
    }
}

// Support parent implementation of virtual functions defined in `gio::ffi::GFileEnumeratorClass` except pairs `xxx_async/xxx_finish` for which GIO provides a default implementation (which should be ok).
// TODO: add parent implementation of `xxx_async/xxx_finish` virtual functions if overriding these default implementations is supported.
pub trait FileEnumeratorImplExt: FileEnumeratorImpl {
    fn parent_next_file(
        &self,
        cancellable: Option<&Cancellable>,
    ) -> Result<Option<FileInfo>, glib::Error> {
        if self.obj().is_closed() {
            Err(glib::Error::new::<IOErrorEnum>(
                IOErrorEnum::Closed,
                "Enumerator is closed",
            ))
        } else {
            unsafe {
                let data = Self::type_data();
                let parent_class = data.as_ref().parent_class() as *const ffi::GFileEnumeratorClass;

                let f = (*parent_class)
                    .next_file
                    .expect("No parent class implementation for \"next_file\"");

                let mut error = std::ptr::null_mut();
                let res = f(
                    self.obj()
                        .unsafe_cast_ref::<FileEnumerator>()
                        .to_glib_none()
                        .0,
                    cancellable.as_ref().to_glib_none().0,
                    &mut error,
                );
                if error.is_null() {
                    Ok(from_glib_full(res))
                } else {
                    Err(from_glib_full(error))
                }
            }
        }
    }

    fn parent_close(&self, cancellable: Option<&Cancellable>) -> (bool, Option<glib::Error>) {
        unsafe {
            let data = Self::type_data();
            let parent_class = data.as_ref().parent_class() as *const ffi::GFileEnumeratorClass;

            let f = (*parent_class)
                .close_fn
                .expect("No parent class implementation for \"close_fn\"");

            let mut error = std::ptr::null_mut();
            let is_ok = f(
                self.obj()
                    .unsafe_cast_ref::<FileEnumerator>()
                    .to_glib_none()
                    .0,
                cancellable.as_ref().to_glib_none().0,
                &mut error,
            );
            (from_glib(is_ok), from_glib_full(error))
        }
    }
}

impl<T: FileEnumeratorImpl> FileEnumeratorImplExt for T {}

// Implement virtual functions defined in `gio::ffi::GFileEnumeratorClass` except pairs `xxx_async/xxx_finish` for which GIO provides a default implementation.
unsafe impl<T: FileEnumeratorImpl> IsSubclassable<T> for FileEnumerator {
    fn class_init(class: &mut ::glib::Class<Self>) {
        Self::parent_class_init::<T>(class);

        let klass = class.as_mut();
        klass.next_file = Some(next_file::<T>);
        klass.close_fn = Some(close_fn::<T>);
        // `GFileEnumerator` already implements `xxx_async/xxx_finish` vfuncs and this should be ok.
        // TODO: when needed, override the `GFileEnumerator` implementation of the following vfuncs:
        // klass.next_files_async = Some(next_files_async::<T>);
        // klass.next_files_finish = Some(next_files_finish::<T>);
        // klass.close_async = Some(close_async::<T>);
        // klass.close_finish = Some(close_finish::<T>);
    }
}

unsafe extern "C" fn next_file<T: FileEnumeratorImpl>(
    enumerator: *mut ffi::GFileEnumerator,
    cancellable: *mut ffi::GCancellable,
    error: *mut *mut glib::ffi::GError,
) -> *mut ffi::GFileInfo {
    let instance = &*(enumerator as *mut T::Instance);
    let imp = instance.imp();
    let cancellable = Option::<Cancellable>::from_glib_none(cancellable);

    let res = imp.next_file(cancellable.as_ref());

    match res {
        Ok(fileinfo) => fileinfo.to_glib_full(),
        Err(err) => {
            if !error.is_null() {
                *error = err.to_glib_full()
            }
            std::ptr::null_mut()
        }
    }
}

unsafe extern "C" fn close_fn<T: FileEnumeratorImpl>(
    enumerator: *mut ffi::GFileEnumerator,
    cancellable: *mut ffi::GCancellable,
    error: *mut *mut glib::ffi::GError,
) -> glib::ffi::gboolean {
    let instance = &*(enumerator as *mut T::Instance);
    let imp = instance.imp();
    let cancellable = Option::<Cancellable>::from_glib_none(cancellable);

    let res = imp.close(cancellable.as_ref());

    if !error.is_null() {
        *error = res.1.to_glib_full()
    }

    res.0.into_glib()
}

#[cfg(test)]
mod tests {
    // The following tests rely on a custom type `MyCustomFileEnumerator` that extends another custom type `MyFileEnumerator`.
    // For each virtual method defined in class `gio::ffi::GFileEnumeratorClass`, a test checks that `MyCustomFileEnumerator` and `MyFileEnumerator` return the same results.

    use super::*;

    // Define `MyCustomFileEnumerator` as a subclass of `MyFileEnumerator`.
    mod imp {
        use std::cell::Cell;

        use super::*;

        #[derive(Default)]
        pub struct MyFileEnumerator(Cell<i32>);

        #[glib::object_subclass]
        impl ObjectSubclass for MyFileEnumerator {
            const NAME: &'static str = "MyFileEnumerator";
            type Type = super::MyFileEnumerator;
            type ParentType = FileEnumerator;
        }

        impl ObjectImpl for MyFileEnumerator {
            fn dispose(&self) {
                let _ = self.obj().close(Cancellable::NONE);
            }
        }

        // Implements `FileEnumeratorImpl` with custom implementation.
        impl FileEnumeratorImpl for MyFileEnumerator {
            fn next_file(
                &self,
                cancellable: Option<&Cancellable>,
            ) -> Result<Option<FileInfo>, glib::Error> {
                if cancellable.is_some_and(|c| c.is_cancelled()) {
                    Err(glib::Error::new::<IOErrorEnum>(
                        IOErrorEnum::Cancelled,
                        "Operation was cancelled",
                    ))
                } else {
                    match self.0.get() {
                        -1 => Err(glib::Error::new::<IOErrorEnum>(
                            IOErrorEnum::Closed,
                            "Enumerator is closed",
                        )),
                        i if i < 3 => {
                            let file_info = FileInfo::new();
                            file_info.set_display_name(&format!("file{i}"));
                            self.0.set(i + 1);
                            Ok(Some(file_info))
                        }
                        _ => Ok(None),
                    }
                }
            }

            fn close(&self, cancellable: Option<&Cancellable>) -> (bool, Option<glib::Error>) {
                if cancellable.is_some_and(|c| c.is_cancelled()) {
                    (
                        false,
                        Some(glib::Error::new::<IOErrorEnum>(
                            IOErrorEnum::Cancelled,
                            "Operation was cancelled",
                        )),
                    )
                } else {
                    self.0.set(-1);
                    (true, None)
                }
            }
        }

        #[derive(Default)]
        pub struct MyCustomFileEnumerator;

        #[glib::object_subclass]
        impl ObjectSubclass for MyCustomFileEnumerator {
            const NAME: &'static str = "MyCustomFileEnumerator";
            type Type = super::MyCustomFileEnumerator;
            type ParentType = super::MyFileEnumerator;
        }

        impl ObjectImpl for MyCustomFileEnumerator {}

        // Implements `FileEnumeratorImpl` with default implementation, which calls the parent's implementation.
        impl FileEnumeratorImpl for MyCustomFileEnumerator {}

        impl MyFileEnumeratorImpl for MyCustomFileEnumerator {}
    }

    glib::wrapper! {
        pub struct MyFileEnumerator(ObjectSubclass<imp::MyFileEnumerator>) @extends FileEnumerator;
    }

    pub trait MyFileEnumeratorImpl:
        ObjectImpl + ObjectSubclass<Type: IsA<MyFileEnumerator> + IsA<FileEnumerator>>
    {
    }

    // To make this class subclassable we need to implement IsSubclassable
    unsafe impl<T: MyFileEnumeratorImpl + FileEnumeratorImpl> IsSubclassable<T> for MyFileEnumerator {}

    glib::wrapper! {
        pub struct MyCustomFileEnumerator(ObjectSubclass<imp::MyCustomFileEnumerator>) @extends MyFileEnumerator, FileEnumerator;
    }

    #[test]
    fn file_enumerator_next_file() {
        // invoke `MyCustomFileEnumerator` implementation of `gio::ffi::GFileEnumeratorClass::next_file`
        let my_custom_file_enumerator = glib::Object::new::<MyCustomFileEnumerator>();
        let res = my_custom_file_enumerator.next_file(Cancellable::NONE);
        assert!(res.as_ref().is_ok_and(|res| res.is_some()));
        let filename = res.unwrap().unwrap().display_name();

        // invoke `MyFileEnumerator` implementation of `gio::ffi::GFileEnumeratorClass::next_file`
        let my_file_enumerator = glib::Object::new::<MyFileEnumerator>();
        let res = my_file_enumerator.next_file(Cancellable::NONE);
        assert!(res.as_ref().is_ok_and(|res| res.is_some()));
        let expected = res.unwrap().unwrap().display_name();

        // both filenames should equal
        assert_eq!(filename, expected);

        // and also next results until there is no more file info
        for res in my_custom_file_enumerator.upcast::<FileEnumerator>() {
            assert!(res.as_ref().is_ok());
            let filename = res.unwrap().display_name();

            let res = my_file_enumerator.next_file(Cancellable::NONE);
            assert!(res.as_ref().is_ok_and(|res| res.is_some()));
            let expected = res.unwrap().unwrap().display_name();

            // both filenames should equal
            assert_eq!(filename, expected);
        }
    }

    #[test]
    fn file_enumerator_close() {
        // invoke `MyCustomFileEnumerator` implementation of `gio::ffi::GFileEnumeratorClass::next_file`
        let my_custom_file_enumerator = glib::Object::new::<MyCustomFileEnumerator>();
        let res = my_custom_file_enumerator.next_file(Cancellable::NONE);
        assert!(res.as_ref().is_ok_and(|res| res.is_some()));
        let filename = res.unwrap().unwrap().display_name();

        // invoke `MyFileEnumerator` implementation of `gio::ffi::GFileEnumeratorClass::next_file`
        let my_file_enumerator = glib::Object::new::<MyFileEnumerator>();
        let res = my_file_enumerator.next_file(Cancellable::NONE);
        assert!(res.as_ref().is_ok_and(|res| res.is_some()));
        let expected = res.unwrap().unwrap().display_name();

        // both filenames should equal
        assert_eq!(filename, expected);

        // invoke `MyCustomFileEnumerator` implementation of `gio::ffi::GFileEnumeratorClass::close`
        let res = my_custom_file_enumerator.close(Cancellable::NONE);
        assert_eq!(res.1, None);
        let closed = res.0;

        // invoke `MyFileEnumerator` implementation of `gio::ffi::GFileEnumeratorClass::close`
        let res = my_file_enumerator.close(Cancellable::NONE);
        assert_eq!(res.1, None);
        let expected = res.0;

        // both results should equal
        assert_eq!(closed, expected);

        // invoke `MyCustomFileEnumerator` implementation of `gio::ffi::GFileEnumeratorClass::next_file`
        let res = my_custom_file_enumerator.next_file(Cancellable::NONE);
        assert!(res.is_err());
        let err = res.unwrap_err();

        // invoke `MyFileEnumerator` implementation of `gio::ffi::GFileEnumeratorClass::next_file`
        let res = my_file_enumerator.next_file(Cancellable::NONE);
        assert!(res.is_err());
        let expected = res.unwrap_err();

        // both errors should equal
        assert_eq!(err.domain(), expected.domain());
        assert!(err.matches::<IOErrorEnum>(IOErrorEnum::Closed));
        assert!(expected.matches::<IOErrorEnum>(IOErrorEnum::Closed));
        assert_eq!(err.message(), expected.message());
    }

    #[test]
    fn file_enumerator_cancel() {
        // invoke `MyCustomFileEnumerator` implementation of `gio::ffi::GFileEnumeratorClass::next_file`
        let my_custom_file_enumerator = glib::Object::new::<MyCustomFileEnumerator>();
        let res = my_custom_file_enumerator.next_file(Cancellable::NONE);
        assert!(res.as_ref().is_ok_and(|res| res.is_some()));
        let filename = res.unwrap().unwrap().display_name();

        // invoke `MyFileEnumerator` implementation of `gio::ffi::GFileEnumeratorClass::next_file`
        let my_file_enumerator = glib::Object::new::<MyFileEnumerator>();
        let res = my_file_enumerator.next_file(Cancellable::NONE);
        assert!(res.as_ref().is_ok_and(|res| res.is_some()));
        let expected = res.unwrap().unwrap().display_name();

        // both filenames should equal
        assert_eq!(filename, expected);

        // invoke `MyCustomFileEnumerator` implementation of `gio::ffi::GFileEnumeratorClass::next_file` with cancel
        let cancellable = Cancellable::new();
        cancellable.cancel();
        let res = my_custom_file_enumerator.next_file(Some(&cancellable));
        assert!(res.as_ref().is_err());
        let err = res.unwrap_err();

        // invoke `MyFileEnumerator` implementation of `gio::ffi::GFileEnumeratorClass::next_file` with cancel
        let cancellable = Cancellable::new();
        cancellable.cancel();
        let res = my_file_enumerator.next_file(Some(&cancellable));
        assert!(res.as_ref().is_err());
        let expected = res.unwrap_err();

        // both errors should equal
        assert_eq!(err.domain(), expected.domain());
        assert!(err.matches::<IOErrorEnum>(IOErrorEnum::Cancelled));
        assert!(expected.matches::<IOErrorEnum>(IOErrorEnum::Cancelled));
        assert_eq!(err.message(), expected.message());

        // invoke `MyCustomFileEnumerator` implementation of `gio::ffi::GFileEnumeratorClass::close` with cancel
        let cancellable = Cancellable::new();
        cancellable.cancel();
        let res = my_custom_file_enumerator.close(Some(&cancellable));
        assert!(res.1.is_some());
        let err = res.1.unwrap();

        // invoke `MyFileEnumerator` implementation of `gio::ffi::GFileEnumeratorClass::close` with cancel
        let cancellable = Cancellable::new();
        cancellable.cancel();
        let res = my_file_enumerator.close(Some(&cancellable));
        assert!(res.1.is_some());
        let expected = res.1.unwrap();

        // both errors should equal
        assert_eq!(err.domain(), expected.domain());
        assert!(err.matches::<IOErrorEnum>(IOErrorEnum::Cancelled));
        assert!(expected.matches::<IOErrorEnum>(IOErrorEnum::Cancelled));
        assert_eq!(err.message(), expected.message());
    }
}
