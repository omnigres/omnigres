extern crate std;

use {
    super::Handle,
    futures::{
        channel::oneshot,
        future::{self, FutureExt},
    },
    std::{
        boxed::Box,
        collections::hash_map::Entry,
        fmt,
        future::{Future, IntoFuture},
        pin::Pin,
        sync::atomic::{AtomicU32, Ordering::Relaxed},
        task::{Context, Poll},
    },
};

#[doc(hidden)]
pub struct FutureVtable<T> {
    pub write: fn(future: u32, value: T) -> Pin<Box<dyn Future<Output = bool>>>,
    pub read: fn(future: u32) -> Pin<Box<dyn Future<Output = Option<T>>>>,
    pub cancel_write: fn(future: u32),
    pub cancel_read: fn(future: u32),
    pub close_writable: fn(future: u32),
    pub close_readable: fn(future: u32),
}

/// Represents the writable end of a Component Model `future`.
pub struct FutureWriter<T: 'static> {
    handle: u32,
    vtable: &'static FutureVtable<T>,
}

impl<T> fmt::Debug for FutureWriter<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("FutureWriter")
            .field("handle", &self.handle)
            .finish()
    }
}

/// Represents a write operation which may be canceled prior to completion.
pub struct CancelableWrite<T: 'static> {
    writer: Option<FutureWriter<T>>,
    future: Pin<Box<dyn Future<Output = ()>>>,
}

impl<T> Future for CancelableWrite<T> {
    type Output = ();

    fn poll(self: Pin<&mut Self>, cx: &mut Context) -> Poll<()> {
        let me = self.get_mut();
        match me.future.poll_unpin(cx) {
            Poll::Ready(()) => {
                me.writer = None;
                Poll::Ready(())
            }
            Poll::Pending => Poll::Pending,
        }
    }
}

impl<T> CancelableWrite<T> {
    /// Cancel this write if it hasn't already completed, returning the original `FutureWriter`.
    ///
    /// This method will panic if the write has already completed.
    pub fn cancel(mut self) -> FutureWriter<T> {
        self.cancel_mut()
    }

    fn cancel_mut(&mut self) -> FutureWriter<T> {
        let writer = self.writer.take().unwrap();
        super::with_entry(writer.handle, |entry| match entry {
            Entry::Vacant(_) => unreachable!(),
            Entry::Occupied(mut entry) => match entry.get() {
                Handle::LocalOpen
                | Handle::LocalWaiting(_)
                | Handle::Read
                | Handle::LocalClosed => unreachable!(),
                Handle::LocalReady(..) => {
                    entry.insert(Handle::LocalOpen);
                }
                Handle::Write => (writer.vtable.cancel_write)(writer.handle),
            },
        });
        writer
    }
}

impl<T> Drop for CancelableWrite<T> {
    fn drop(&mut self) {
        if self.writer.is_some() {
            self.cancel_mut();
        }
    }
}

impl<T> FutureWriter<T> {
    #[doc(hidden)]
    pub fn new(handle: u32, vtable: &'static FutureVtable<T>) -> Self {
        Self { handle, vtable }
    }

    /// Write the specified value to this `future`.
    pub fn write(self, v: T) -> CancelableWrite<T> {
        let handle = self.handle;
        let vtable = self.vtable;
        CancelableWrite {
            writer: Some(self),
            future: super::with_entry(handle, |entry| match entry {
                Entry::Vacant(_) => unreachable!(),
                Entry::Occupied(mut entry) => match entry.get() {
                    Handle::LocalOpen => {
                        let mut v = Some(v);
                        Box::pin(future::poll_fn(move |cx| {
                            super::with_entry(handle, |entry| match entry {
                                Entry::Vacant(_) => unreachable!(),
                                Entry::Occupied(mut entry) => match entry.get() {
                                    Handle::LocalOpen => {
                                        entry.insert(Handle::LocalReady(
                                            Box::new(v.take().unwrap()),
                                            cx.waker().clone(),
                                        ));
                                        Poll::Pending
                                    }
                                    Handle::LocalReady(..) => Poll::Pending,
                                    Handle::LocalClosed => Poll::Ready(()),
                                    Handle::LocalWaiting(_) | Handle::Read | Handle::Write => {
                                        unreachable!()
                                    }
                                },
                            })
                        })) as Pin<Box<dyn Future<Output = _>>>
                    }
                    Handle::LocalWaiting(_) => {
                        let Handle::LocalWaiting(tx) = entry.insert(Handle::LocalClosed) else {
                            unreachable!()
                        };
                        _ = tx.send(Box::new(v));
                        Box::pin(future::ready(()))
                    }
                    Handle::LocalClosed => Box::pin(future::ready(())),
                    Handle::Read | Handle::LocalReady(..) => unreachable!(),
                    Handle::Write => Box::pin((vtable.write)(handle, v).map(drop)),
                },
            }),
        }
    }
}

impl<T> Drop for FutureWriter<T> {
    fn drop(&mut self) {
        super::with_entry(self.handle, |entry| match entry {
            Entry::Vacant(_) => unreachable!(),
            Entry::Occupied(mut entry) => match entry.get_mut() {
                Handle::LocalOpen | Handle::LocalWaiting(_) | Handle::LocalReady(..) => {
                    entry.insert(Handle::LocalClosed);
                }
                Handle::Read => unreachable!(),
                Handle::Write | Handle::LocalClosed => {
                    entry.remove();
                    (self.vtable.close_writable)(self.handle);
                }
            },
        });
    }
}

/// Represents a read operation which may be canceled prior to completion.
pub struct CancelableRead<T: 'static> {
    reader: Option<FutureReader<T>>,
    future: Pin<Box<dyn Future<Output = Option<T>>>>,
}

impl<T> Future for CancelableRead<T> {
    type Output = Option<T>;

    fn poll(self: Pin<&mut Self>, cx: &mut Context) -> Poll<Option<T>> {
        let me = self.get_mut();
        match me.future.poll_unpin(cx) {
            Poll::Ready(v) => {
                me.reader = None;
                Poll::Ready(v)
            }
            Poll::Pending => Poll::Pending,
        }
    }
}

impl<T> CancelableRead<T> {
    /// Cancel this read if it hasn't already completed, returning the original `FutureReader`.
    ///
    /// This method will panic if the read has already completed.
    pub fn cancel(mut self) -> FutureReader<T> {
        self.cancel_mut()
    }

    fn cancel_mut(&mut self) -> FutureReader<T> {
        let reader = self.reader.take().unwrap();
        let handle = reader.handle.load(Relaxed);
        super::with_entry(handle, |entry| match entry {
            Entry::Vacant(_) => unreachable!(),
            Entry::Occupied(mut entry) => match entry.get() {
                Handle::LocalOpen
                | Handle::LocalReady(..)
                | Handle::Write
                | Handle::LocalClosed => unreachable!(),
                Handle::LocalWaiting(_) => {
                    entry.insert(Handle::LocalOpen);
                }
                Handle::Read => (reader.vtable.cancel_read)(handle),
            },
        });
        reader
    }
}

impl<T> Drop for CancelableRead<T> {
    fn drop(&mut self) {
        if self.reader.is_some() {
            self.cancel_mut();
        }
    }
}

/// Represents the readable end of a Component Model `future`.
pub struct FutureReader<T: 'static> {
    handle: AtomicU32,
    vtable: &'static FutureVtable<T>,
}

impl<T> fmt::Debug for FutureReader<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("FutureReader")
            .field("handle", &self.handle)
            .finish()
    }
}

impl<T> FutureReader<T> {
    #[doc(hidden)]
    pub fn new(handle: u32, vtable: &'static FutureVtable<T>) -> Self {
        Self {
            handle: AtomicU32::new(handle),
            vtable,
        }
    }

    #[doc(hidden)]
    pub fn from_handle_and_vtable(handle: u32, vtable: &'static FutureVtable<T>) -> Self {
        super::with_entry(handle, |entry| match entry {
            Entry::Vacant(entry) => {
                entry.insert(Handle::Read);
            }
            Entry::Occupied(mut entry) => match entry.get() {
                Handle::Write => {
                    entry.insert(Handle::LocalOpen);
                }
                Handle::Read
                | Handle::LocalOpen
                | Handle::LocalReady(..)
                | Handle::LocalWaiting(_)
                | Handle::LocalClosed => {
                    unreachable!()
                }
            },
        });

        Self {
            handle: AtomicU32::new(handle),
            vtable,
        }
    }

    #[doc(hidden)]
    pub fn take_handle(&self) -> u32 {
        let handle = self.handle.swap(u32::MAX, Relaxed);
        super::with_entry(handle, |entry| match entry {
            Entry::Vacant(_) => unreachable!(),
            Entry::Occupied(mut entry) => match entry.get() {
                Handle::LocalOpen => {
                    entry.insert(Handle::Write);
                }
                Handle::Read | Handle::LocalClosed => {
                    entry.remove();
                }
                Handle::LocalReady(..) | Handle::LocalWaiting(_) | Handle::Write => unreachable!(),
            },
        });

        handle
    }
}

impl<T> IntoFuture for FutureReader<T> {
    type Output = Option<T>;
    type IntoFuture = CancelableRead<T>;

    /// Convert this object into a `Future` which will resolve when a value is
    /// written to the writable end of this `future` (yielding a `Some` result)
    /// or when the writable end is dropped (yielding a `None` result).
    fn into_future(self) -> Self::IntoFuture {
        let handle = self.handle.load(Relaxed);
        let vtable = self.vtable;
        CancelableRead {
            reader: Some(self),
            future: super::with_entry(handle, |entry| match entry {
                Entry::Vacant(_) => unreachable!(),
                Entry::Occupied(mut entry) => match entry.get() {
                    Handle::Write | Handle::LocalWaiting(_) => unreachable!(),
                    Handle::Read => Box::pin(async move { (vtable.read)(handle).await })
                        as Pin<Box<dyn Future<Output = _>>>,
                    Handle::LocalOpen => {
                        let (tx, rx) = oneshot::channel();
                        entry.insert(Handle::LocalWaiting(tx));
                        Box::pin(async move { rx.await.ok().map(|v| *v.downcast().unwrap()) })
                    }
                    Handle::LocalClosed => Box::pin(future::ready(None)),
                    Handle::LocalReady(..) => {
                        let Handle::LocalReady(v, waker) = entry.insert(Handle::LocalClosed) else {
                            unreachable!()
                        };
                        waker.wake();
                        Box::pin(future::ready(Some(*v.downcast().unwrap())))
                    }
                },
            }),
        }
    }
}

impl<T> Drop for FutureReader<T> {
    fn drop(&mut self) {
        match self.handle.load(Relaxed) {
            u32::MAX => {}
            handle => {
                super::with_entry(handle, |entry| match entry {
                    Entry::Vacant(_) => unreachable!(),
                    Entry::Occupied(mut entry) => match entry.get_mut() {
                        Handle::LocalReady(..) => {
                            let Handle::LocalReady(_, waker) = entry.insert(Handle::LocalClosed)
                            else {
                                unreachable!()
                            };
                            waker.wake();
                        }
                        Handle::LocalOpen | Handle::LocalWaiting(_) => {
                            entry.insert(Handle::LocalClosed);
                        }
                        Handle::Read | Handle::LocalClosed => {
                            entry.remove();
                            (self.vtable.close_readable)(handle);
                        }
                        Handle::Write => unreachable!(),
                    },
                });
            }
        }
    }
}
