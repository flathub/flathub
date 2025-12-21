#![no_std]
#![cfg_attr(fieldoffset_assert_in_const_fn, feature(const_panic))]
// Explicit lifetimes are clearer when we are working with raw pointers,
// as the compiler will not warn us if we specify lifetime constraints
// which are too lax.
#![allow(clippy::needless_lifetimes)]

#[cfg(all(test, fieldoffset_has_alloc))]
extern crate alloc;

use core::fmt;
use core::marker::PhantomData;
use core::mem;
use core::ops::Add;
use core::pin::Pin;

#[doc(hidden)]
pub extern crate memoffset as __memoffset; // `pub` for macro availability

/// Represents a pointer to a field of type `U` within the type `T`
///
/// The `PinFlag` parameter can be set to `AllowPin` to enable the projection
/// from Pin<&T> to Pin<&U>
#[repr(transparent)]
pub struct FieldOffset<T, U, PinFlag = NotPinned>(
    /// Offset in bytes of the field within the struct
    usize,
    /// A pointer-to-member can be thought of as a function from
    /// `&T` to `&U` with matching lifetimes
    ///
    /// ```compile_fail
    /// use field_offset::FieldOffset;
    /// struct Foo<'a>(&'a str);
    /// fn test<'a>(foo: &Foo<'a>, of: FieldOffset<Foo<'static>, &'static str>) -> &'static str {
    ///     let of2 : FieldOffset<Foo<'a>, &'static str> = of; // This must not compile
    ///     of2.apply(foo)
    /// }
    /// ```
    /// That should compile:
    /// ```
    /// use field_offset::FieldOffset;
    /// struct Foo<'a>(&'a str, &'static str);
    /// fn test<'a>(foo: &'a Foo<'static>, of: FieldOffset<Foo, &'static str>) -> &'a str {
    ///     let of2 : FieldOffset<Foo<'static>, &'static str> = of;
    ///     of.apply(foo)
    /// }
    /// fn test2(foo: &Foo<'static>, of: FieldOffset<Foo, &'static str>) -> &'static str {
    ///     let of2 : FieldOffset<Foo<'static>, &'static str> = of;
    ///     of.apply(foo)
    /// }
    /// fn test3<'a>(foo: &'a Foo, of: FieldOffset<Foo<'a>, &'a str>) -> &'a str {
    ///     of.apply(foo)
    /// }
    /// ```
    PhantomData<(PhantomContra<T>, U, PinFlag)>,
);

/// `fn` cannot appear directly in a type that need to be const.
/// Workaround that with an indirection
struct PhantomContra<T>(fn(T));

/// Type that can be used in the `PinFlag` parameter of `FieldOffset` to specify that
/// this projection is valid on Pin types.
/// See documentation of `FieldOffset::new_from_offset_pinned`
pub enum AllowPin {}

/// Type that can be used in the `PinFlag` parameter of `FieldOffset` to specify that
/// this projection is not valid on Pin types.
pub enum NotPinned {}

impl<T, U> FieldOffset<T, U, NotPinned> {
    // Use MaybeUninit to get a fake T
    #[cfg(fieldoffset_maybe_uninit)]
    #[inline]
    fn with_uninit_ptr<R, F: FnOnce(*const T) -> R>(f: F) -> R {
        let uninit = mem::MaybeUninit::<T>::uninit();
        f(uninit.as_ptr())
    }

    // Use a dangling pointer to get a fake T
    #[cfg(not(fieldoffset_maybe_uninit))]
    #[inline]
    fn with_uninit_ptr<R, F: FnOnce(*const T) -> R>(f: F) -> R {
        f(mem::align_of::<T>() as *const T)
    }

    /// Construct a field offset via a lambda which returns a reference
    /// to the field in question.
    ///
    /// # Safety
    ///
    /// The lambda *must not* dereference the provided pointer or access the
    /// inner value in any way as it may point to uninitialized memory.
    ///
    /// For the returned `FieldOffset` to be safe to use, the returned pointer
    /// must be valid for *any* instance of `T`. For example, returning a pointer
    /// to a field from an enum with multiple variants will produce a `FieldOffset`
    /// which is unsafe to use.
    pub unsafe fn new<F: for<'a> FnOnce(*const T) -> *const U>(f: F) -> Self {
        let offset = Self::with_uninit_ptr(|base_ptr| {
            let field_ptr = f(base_ptr);
            (field_ptr as usize).wrapping_sub(base_ptr as usize)
        });

        // Construct an instance using the offset
        Self::new_from_offset(offset)
    }
    /// Construct a field offset directly from a byte offset.
    ///
    /// # Safety
    ///
    /// For the returned `FieldOffset` to be safe to use, the field offset
    /// must be valid for *any* instance of `T`. For example, returning the offset
    /// to a field from an enum with multiple variants will produce a `FieldOffset`
    /// which is unsafe to use.
    #[inline]
    pub const unsafe fn new_from_offset(offset: usize) -> Self {
        // Sanity check: ensure that the field offset plus the field size
        // is no greater than the size of the containing struct. This is
        // not sufficient to make the function *safe*, but it does catch
        // obvious errors like returning a reference to a boxed value,
        // which is owned by `T` and so has the correct lifetime, but is not
        // actually a field.
        #[cfg(fieldoffset_assert_in_const_fn)]
        assert!(offset + mem::size_of::<U>() <= mem::size_of::<T>());
        // On stable rust, we can still get an assert in debug mode,
        // relying on the checked overflow behaviour
        let _ = mem::size_of::<T>() - (offset + mem::size_of::<U>());

        FieldOffset(offset, PhantomData)
    }
}

// Methods for applying the pointer to member
impl<T, U, PinFlag> FieldOffset<T, U, PinFlag> {
    /// Apply the field offset to a native pointer.
    #[inline]
    pub fn apply_ptr(self, x: *const T) -> *const U {
        ((x as usize) + self.0) as *const U
    }
    /// Apply the field offset to a native mutable pointer.
    #[inline]
    pub fn apply_ptr_mut(self, x: *mut T) -> *mut U {
        ((x as usize) + self.0) as *mut U
    }
    /// Apply the field offset to a reference.
    #[inline]
    pub fn apply<'a>(self, x: &'a T) -> &'a U {
        unsafe { &*self.apply_ptr(x) }
    }
    /// Apply the field offset to a mutable reference.
    #[inline]
    pub fn apply_mut<'a>(self, x: &'a mut T) -> &'a mut U {
        unsafe { &mut *self.apply_ptr_mut(x) }
    }
    /// Get the raw byte offset for this field offset.
    #[inline]
    pub const fn get_byte_offset(self) -> usize {
        self.0
    }

    // Methods for unapplying the pointer to member

    /// Unapply the field offset to a native pointer.
    ///
    /// # Safety
    ///
    /// *Warning: very unsafe!*
    ///
    /// This applies a negative offset to a pointer. If the safety
    /// implications of this are not already clear to you, then *do
    /// not* use this method. Also be aware that Rust has stronger
    /// aliasing rules than other languages, so it may be UB to
    /// dereference the resulting pointer even if it points to a valid
    /// location, due to the presence of other live references.
    #[inline]
    pub unsafe fn unapply_ptr(self, x: *const U) -> *const T {
        ((x as usize) - self.0) as *const T
    }
    /// Unapply the field offset to a native mutable pointer.
    ///
    /// # Safety
    ///
    /// *Warning: very unsafe!*
    ///
    /// This applies a negative offset to a pointer. If the safety
    /// implications of this are not already clear to you, then *do
    /// not* use this method. Also be aware that Rust has stronger
    /// aliasing rules than other languages, so it may be UB to
    /// dereference the resulting pointer even if it points to a valid
    /// location, due to the presence of other live references.
    #[inline]
    pub unsafe fn unapply_ptr_mut(self, x: *mut U) -> *mut T {
        ((x as usize) - self.0) as *mut T
    }
    /// Unapply the field offset to a reference.
    ///
    /// # Safety
    ///
    /// *Warning: very unsafe!*
    ///
    /// This applies a negative offset to a reference. If the safety
    /// implications of this are not already clear to you, then *do
    /// not* use this method. Also be aware that Rust has stronger
    /// aliasing rules than other languages, so this method may cause UB
    /// even if the resulting reference points to a valid location, due
    /// to the presence of other live references.
    #[inline]
    pub unsafe fn unapply<'a>(self, x: &'a U) -> &'a T {
        &*self.unapply_ptr(x)
    }
    /// Unapply the field offset to a mutable reference.
    ///
    /// # Safety
    ///
    /// *Warning: very unsafe!*
    ///
    /// This applies a negative offset to a reference. If the safety
    /// implications of this are not already clear to you, then *do
    /// not* use this method. Also be aware that Rust has stronger
    /// aliasing rules than other languages, so this method may cause UB
    /// even if the resulting reference points to a valid location, due
    /// to the presence of other live references.
    #[inline]
    pub unsafe fn unapply_mut<'a>(self, x: &'a mut U) -> &'a mut T {
        &mut *self.unapply_ptr_mut(x)
    }

    /// Convert this offset to an offset that is allowed to go from `Pin<&T>`
    /// to `Pin<&U>`
    ///
    /// # Safety
    ///
    /// The Pin safety rules for projection must be respected. These rules are
    /// explained in the
    /// [Pin documentation](https://doc.rust-lang.org/stable/std/pin/index.html#pinning-is-structural-for-field)
    pub const unsafe fn as_pinned_projection(self) -> FieldOffset<T, U, AllowPin> {
        FieldOffset::new_from_offset_pinned(self.get_byte_offset())
    }

    /// Remove the AllowPin flag
    pub const fn as_unpinned_projection(self) -> FieldOffset<T, U, NotPinned> {
        unsafe { FieldOffset::new_from_offset(self.get_byte_offset()) }
    }
}

impl<T, U> FieldOffset<T, U, AllowPin> {
    /// Construct a field offset directly from a byte offset, which can be projected from
    /// a pinned.
    ///
    /// # Safety
    ///
    /// In addition to the safety rules of FieldOffset::new_from_offset, the projection
    /// from `Pin<&T>` to `Pin<&U>` must also be allowed. The rules are explained in the
    /// [Pin documentation](https://doc.rust-lang.org/stable/std/pin/index.html#pinning-is-structural-for-field)
    #[inline]
    pub const unsafe fn new_from_offset_pinned(offset: usize) -> Self {
        FieldOffset(offset, PhantomData)
    }

    /// Apply the field offset to a pinned reference and return a pinned
    /// reference to the field
    #[inline]
    pub fn apply_pin<'a>(self, x: Pin<&'a T>) -> Pin<&'a U> {
        unsafe { x.map_unchecked(|x| self.apply(x)) }
    }
    /// Apply the field offset to a pinned mutable reference and return a
    /// pinned mutable reference to the field
    #[inline]
    pub fn apply_pin_mut<'a>(self, x: Pin<&'a mut T>) -> Pin<&'a mut U> {
        unsafe { x.map_unchecked_mut(|x| self.apply_mut(x)) }
    }
}

impl<T, U> From<FieldOffset<T, U, AllowPin>> for FieldOffset<T, U, NotPinned> {
    fn from(other: FieldOffset<T, U, AllowPin>) -> Self {
        other.as_unpinned_projection()
    }
}

/// Allow chaining pointer-to-members.
///
/// Applying the resulting field offset is equivalent to applying the first
/// field offset, then applying the second field offset.
///
/// The requirements on the generic type parameters ensure this is a safe operation.
impl<T, U, V> Add<FieldOffset<U, V>> for FieldOffset<T, U> {
    type Output = FieldOffset<T, V>;
    #[inline]
    fn add(self, other: FieldOffset<U, V>) -> FieldOffset<T, V> {
        FieldOffset(self.0 + other.0, PhantomData)
    }
}
impl<T, U, V> Add<FieldOffset<U, V, AllowPin>> for FieldOffset<T, U, AllowPin> {
    type Output = FieldOffset<T, V, AllowPin>;
    #[inline]
    fn add(self, other: FieldOffset<U, V, AllowPin>) -> FieldOffset<T, V, AllowPin> {
        FieldOffset(self.0 + other.0, PhantomData)
    }
}
impl<T, U, V> Add<FieldOffset<U, V>> for FieldOffset<T, U, AllowPin> {
    type Output = FieldOffset<T, V>;
    #[inline]
    fn add(self, other: FieldOffset<U, V>) -> FieldOffset<T, V> {
        FieldOffset(self.0 + other.0, PhantomData)
    }
}
impl<T, U, V> Add<FieldOffset<U, V, AllowPin>> for FieldOffset<T, U> {
    type Output = FieldOffset<T, V>;
    #[inline]
    fn add(self, other: FieldOffset<U, V, AllowPin>) -> FieldOffset<T, V> {
        FieldOffset(self.0 + other.0, PhantomData)
    }
}

/// The debug implementation prints the byte offset of the field in hexadecimal.
impl<T, U, Flag> fmt::Debug for FieldOffset<T, U, Flag> {
    fn fmt(&self, f: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        write!(f, "FieldOffset({:#x})", self.0)
    }
}

impl<T, U, Flag> Copy for FieldOffset<T, U, Flag> {}
impl<T, U, Flag> Clone for FieldOffset<T, U, Flag> {
    fn clone(&self) -> Self {
        *self
    }
}

/// This macro allows safe construction of a FieldOffset,
/// by generating a known to be valid lambda to pass to the
/// constructor. It takes a type, and the identifier of a field
/// within that type as input.
///
/// Examples:
///
/// Offset of field `Foo.bar`
///
/// ```rust
/// # #[macro_use]
/// # extern crate field_offset;
/// # fn main() {
/// #[repr(C)]
/// struct Foo { foo: i32, bar: i32 }
/// assert_eq!(offset_of!(Foo => bar).get_byte_offset(), 4);
/// # }
/// ```
///
/// Offset of nested field `Foo.bar.x`
///
/// ```rust
/// # #[macro_use]
/// # extern crate field_offset;
/// # fn main() {
/// struct Bar { a: u8, x: u8 }
/// struct Foo { foo: i32, bar: Bar }
/// assert_eq!(offset_of!(Foo => bar: Bar => x).get_byte_offset(), 5);
/// # }
/// ```
#[macro_export]
macro_rules! offset_of {
    ($t: path => $f: tt) => {{
        // Construct the offset
        #[allow(unused_unsafe)]
        unsafe {
            $crate::FieldOffset::<$t, _>::new(|x| {
                $crate::__memoffset::raw_field!(x, $t, $f)
            })
        }
    }};
    ($t: path => $f: ident: $($rest: tt)*) => {
        offset_of!($t => $f) + offset_of!($($rest)*)
    };
}

#[cfg(test)]
mod tests {
    // Example structs
    #[derive(Debug)]
    struct Foo {
        a: u32,
        b: f64,
        c: bool,
    }

    #[derive(Debug)]
    struct Bar {
        x: u32,
        y: Foo,
    }

    #[derive(Debug)]
    struct Tuple(i32, f64);

    #[test]
    fn test_simple() {
        // Get a pointer to `b` within `Foo`
        let foo_b = offset_of!(Foo => b);

        // Construct an example `Foo`
        let mut x = Foo {
            a: 1,
            b: 2.0,
            c: false,
        };

        // Apply the pointer to get at `b` and read it
        {
            let y = foo_b.apply(&x);
            assert_eq!(*y, 2.0);
        }

        // Apply the pointer to get at `b` and mutate it
        {
            let y = foo_b.apply_mut(&mut x);
            *y = 42.0;
        }
        assert_eq!(x.b, 42.0);
    }

    #[test]
    fn test_tuple() {
        // Get a pointer to `b` within `Foo`
        let tuple_1 = offset_of!(Tuple => 1);

        // Construct an example `Foo`
        let mut x = Tuple(1, 42.0);

        // Apply the pointer to get at `b` and read it
        {
            let y = tuple_1.apply(&x);
            assert_eq!(*y, 42.0);
        }

        // Apply the pointer to get at `b` and mutate it
        {
            let y = tuple_1.apply_mut(&mut x);
            *y = 5.0;
        }
        assert_eq!(x.1, 5.0);
    }

    #[test]
    fn test_nested() {
        // Construct an example `Foo`
        let mut x = Bar {
            x: 0,
            y: Foo {
                a: 1,
                b: 2.0,
                c: false,
            },
        };

        // Combine the pointer-to-members
        let bar_y_b = offset_of!(Bar => y: Foo => b);

        // Apply the pointer to get at `b` and mutate it
        {
            let y = bar_y_b.apply_mut(&mut x);
            *y = 42.0;
        }
        assert_eq!(x.y.b, 42.0);
    }

    struct Parameterized<T, U> {
        x: T,
        _y: U,
    }
    #[test]
    fn test_type_parameter() {
        let _ = offset_of!(Parameterized<Parameterized<bool, bool>, bool> => x: Parameterized<bool, bool> => x);
    }

    #[test]
    fn test_const() {
        use crate::FieldOffset;
        #[repr(C)]
        struct SomeStruct {
            a: u8,
            b: u32,
        }
        const CONST_FIELD_OFFSET: FieldOffset<SomeStruct, u32> =
            unsafe { FieldOffset::new_from_offset(4) };
        const CONST_VALUE: usize = CONST_FIELD_OFFSET.get_byte_offset();
        assert_eq!(offset_of!(SomeStruct => b).get_byte_offset(), CONST_VALUE);

        static STATIC_FIELD_OFFSET: FieldOffset<SomeStruct, u32> =
            unsafe { FieldOffset::new_from_offset(4) };
        assert_eq!(
            offset_of!(SomeStruct => b).get_byte_offset(),
            STATIC_FIELD_OFFSET.get_byte_offset()
        );
    }

    #[cfg(fieldoffset_has_alloc)]
    #[test]
    fn test_pin() {
        use alloc::boxed::Box;
        use core::pin::Pin;

        // Get a pointer to `b` within `Foo`
        let foo_b = offset_of!(Foo => b);
        let foo_b_pin = unsafe { foo_b.as_pinned_projection() };
        let foo = Box::pin(Foo {
            a: 21,
            b: 22.0,
            c: true,
        });
        let pb: Pin<&f64> = foo_b_pin.apply_pin(foo.as_ref());
        assert_eq!(*pb, 22.0);

        let mut x = Box::pin(Bar {
            x: 0,
            y: Foo {
                a: 1,
                b: 52.0,
                c: false,
            },
        });
        let bar_y_b = offset_of!(Bar => y: Foo => b);
        assert!(*bar_y_b.apply(&*x) == 52.0);

        let bar_y_pin = unsafe { offset_of!(Bar => y).as_pinned_projection() };
        *(bar_y_pin + foo_b_pin).apply_pin_mut(x.as_mut()) = 12.;
        assert_eq!(x.y.b, 12.0);
    }
}
