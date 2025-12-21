use std::fmt;
use std::future::Future;
use std::mem::ManuallyDrop;
use std::pin::Pin;
use std::sync::Arc;
use std::task::{Context, Poll};

use super::raw::{RawRead, RawUpgradableRead, RawUpgrade, RawWrite};
use super::{
    RwLock, RwLockReadGuard, RwLockReadGuardArc, RwLockUpgradableReadGuard,
    RwLockUpgradableReadGuardArc, RwLockWriteGuard, RwLockWriteGuardArc,
};

/// The future returned by [`RwLock::read`].
pub struct Read<'a, T: ?Sized> {
    /// Raw read lock acquisition future, doesn't depend on `T`.
    pub(super) raw: RawRead<'a>,

    /// Pointer to the value protected by the lock. Covariant in `T`.
    pub(super) value: *const T,
}

unsafe impl<T: Sync + ?Sized> Send for Read<'_, T> {}
unsafe impl<T: Sync + ?Sized> Sync for Read<'_, T> {}

impl<T: ?Sized> fmt::Debug for Read<'_, T> {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("Read { .. }")
    }
}

impl<T: ?Sized> Unpin for Read<'_, T> {}

impl<'a, T: ?Sized> Future for Read<'a, T> {
    type Output = RwLockReadGuard<'a, T>;

    #[inline]
    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        ready!(Pin::new(&mut self.raw).poll(cx));

        Poll::Ready(RwLockReadGuard {
            lock: self.raw.lock,
            value: self.value,
        })
    }
}

/// The future returned by [`RwLock::read_arc`].
pub struct ReadArc<'a, T> {
    /// Raw read lock acquisition future, doesn't depend on `T`.
    pub(super) raw: RawRead<'a>,

    // FIXME: Could be covariant in T
    pub(super) lock: &'a Arc<RwLock<T>>,
}

unsafe impl<T: Send + Sync> Send for ReadArc<'_, T> {}
unsafe impl<T: Send + Sync> Sync for ReadArc<'_, T> {}

impl<T> fmt::Debug for ReadArc<'_, T> {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("ReadArc { .. }")
    }
}

impl<T> Unpin for ReadArc<'_, T> {}

impl<'a, T> Future for ReadArc<'a, T> {
    type Output = RwLockReadGuardArc<T>;

    #[inline]
    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        ready!(Pin::new(&mut self.raw).poll(cx));

        // SAFETY: we just acquired a read lock
        Poll::Ready(unsafe { RwLockReadGuardArc::from_arc(self.lock.clone()) })
    }
}

/// The future returned by [`RwLock::upgradable_read`].
pub struct UpgradableRead<'a, T: ?Sized> {
    /// Raw upgradable read lock acquisition future, doesn't depend on `T`.
    pub(super) raw: RawUpgradableRead<'a>,

    /// Pointer to the value protected by the lock. Invariant in `T`
    /// as the upgradable lock could provide write access.
    pub(super) value: *mut T,
}

unsafe impl<T: Send + Sync + ?Sized> Send for UpgradableRead<'_, T> {}
unsafe impl<T: Sync + ?Sized> Sync for UpgradableRead<'_, T> {}

impl<T: ?Sized> fmt::Debug for UpgradableRead<'_, T> {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("UpgradableRead { .. }")
    }
}

impl<T: ?Sized> Unpin for UpgradableRead<'_, T> {}

impl<'a, T: ?Sized> Future for UpgradableRead<'a, T> {
    type Output = RwLockUpgradableReadGuard<'a, T>;

    #[inline]
    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        ready!(Pin::new(&mut self.raw).poll(cx));

        Poll::Ready(RwLockUpgradableReadGuard {
            lock: self.raw.lock,
            value: self.value,
        })
    }
}

/// The future returned by [`RwLock::upgradable_read_arc`].
pub struct UpgradableReadArc<'a, T: ?Sized> {
    /// Raw upgradable read lock acquisition future, doesn't depend on `T`.
    pub(super) raw: RawUpgradableRead<'a>,

    pub(super) lock: &'a Arc<RwLock<T>>,
}

unsafe impl<T: Send + Sync + ?Sized> Send for UpgradableReadArc<'_, T> {}
unsafe impl<T: Send + Sync + ?Sized> Sync for UpgradableReadArc<'_, T> {}

impl<T: ?Sized> fmt::Debug for UpgradableReadArc<'_, T> {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("UpgradableReadArc { .. }")
    }
}

impl<T: ?Sized> Unpin for UpgradableReadArc<'_, T> {}

impl<'a, T: ?Sized> Future for UpgradableReadArc<'a, T> {
    type Output = RwLockUpgradableReadGuardArc<T>;

    #[inline]
    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        ready!(Pin::new(&mut self.raw).poll(cx));
        Poll::Ready(RwLockUpgradableReadGuardArc {
            lock: self.lock.clone(),
        })
    }
}

/// The future returned by [`RwLock::write`].
pub struct Write<'a, T: ?Sized> {
    /// Raw write lock acquisition future, doesn't depend on `T`.
    pub(super) raw: RawWrite<'a>,

    /// Pointer to the value protected by the lock. Invariant in `T`.
    pub(super) value: *mut T,
}

unsafe impl<T: Send + ?Sized> Send for Write<'_, T> {}
unsafe impl<T: Sync + ?Sized> Sync for Write<'_, T> {}

impl<T: ?Sized> fmt::Debug for Write<'_, T> {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("Write { .. }")
    }
}

impl<T: ?Sized> Unpin for Write<'_, T> {}

impl<'a, T: ?Sized> Future for Write<'a, T> {
    type Output = RwLockWriteGuard<'a, T>;

    #[inline]
    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        ready!(Pin::new(&mut self.raw).poll(cx));

        Poll::Ready(RwLockWriteGuard {
            lock: self.raw.lock,
            value: self.value,
        })
    }
}

/// The future returned by [`RwLock::write_arc`].
pub struct WriteArc<'a, T: ?Sized> {
    /// Raw write lock acquisition future, doesn't depend on `T`.
    pub(super) raw: RawWrite<'a>,

    pub(super) lock: &'a Arc<RwLock<T>>,
}

unsafe impl<T: Send + Sync + ?Sized> Send for WriteArc<'_, T> {}
unsafe impl<T: Send + Sync + ?Sized> Sync for WriteArc<'_, T> {}

impl<T: ?Sized> fmt::Debug for WriteArc<'_, T> {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("WriteArc { .. }")
    }
}

impl<T: ?Sized> Unpin for WriteArc<'_, T> {}

impl<'a, T: ?Sized> Future for WriteArc<'a, T> {
    type Output = RwLockWriteGuardArc<T>;

    #[inline]
    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        ready!(Pin::new(&mut self.raw).poll(cx));

        Poll::Ready(RwLockWriteGuardArc {
            lock: self.lock.clone(),
        })
    }
}

/// The future returned by [`RwLockUpgradableReadGuard::upgrade`].
pub struct Upgrade<'a, T: ?Sized> {
    /// Raw read lock upgrade future, doesn't depend on `T`.
    pub(super) raw: RawUpgrade<'a>,

    /// Pointer to the value protected by the lock. Invariant in `T`.
    pub(super) value: *mut T,
}

unsafe impl<T: Send + ?Sized> Send for Upgrade<'_, T> {}
unsafe impl<T: Sync + ?Sized> Sync for Upgrade<'_, T> {}

impl<T: ?Sized> fmt::Debug for Upgrade<'_, T> {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Upgrade").finish()
    }
}

impl<T: ?Sized> Unpin for Upgrade<'_, T> {}

impl<'a, T: ?Sized> Future for Upgrade<'a, T> {
    type Output = RwLockWriteGuard<'a, T>;

    #[inline]
    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        let lock = ready!(Pin::new(&mut self.raw).poll(cx));

        Poll::Ready(RwLockWriteGuard {
            lock,
            value: self.value,
        })
    }
}

/// The future returned by [`RwLockUpgradableReadGuardArc::upgrade`].
pub struct UpgradeArc<T: ?Sized> {
    /// Raw read lock upgrade future, doesn't depend on `T`.
    /// `'static` is a lie, this field is actually referencing the
    /// `Arc` data. But since this struct also stores said `Arc`, we know
    /// this value will be alive as long as the struct is.
    ///
    /// Yes, one field of the `ArcUpgrade` struct is referencing another.
    /// Such self-references are usually not sound without pinning.
    /// However, in this case, there is an indirection via the heap;
    /// moving the `ArcUpgrade` won't move the heap allocation of the `Arc`,
    /// so the reference inside `RawUpgrade` isn't invalidated.
    pub(super) raw: ManuallyDrop<RawUpgrade<'static>>,

    /// Pointer to the value protected by the lock. Invariant in `T`.
    pub(super) lock: ManuallyDrop<Arc<RwLock<T>>>,
}

impl<T: ?Sized> fmt::Debug for UpgradeArc<T> {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("ArcUpgrade").finish()
    }
}

impl<T: ?Sized> Unpin for UpgradeArc<T> {}

impl<T: ?Sized> Future for UpgradeArc<T> {
    type Output = RwLockWriteGuardArc<T>;

    #[inline]
    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        ready!(Pin::new(&mut *self.raw).poll(cx));

        Poll::Ready(RwLockWriteGuardArc {
            lock: unsafe { ManuallyDrop::take(&mut self.lock) },
        })
    }
}

impl<T: ?Sized> Drop for UpgradeArc<T> {
    #[inline]
    fn drop(&mut self) {
        if !self.raw.is_ready() {
            // SAFETY: we drop the `Arc` (decrementing the reference count)
            // only if this future was cancelled before returning an
            // upgraded lock.
            unsafe {
                ManuallyDrop::drop(&mut self.raw);
                ManuallyDrop::drop(&mut self.lock);
            };
        }
    }
}
