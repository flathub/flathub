//! Async interface for working with processes.
//!
//! This crate is an async version of [`std::process`].
//!
//! # Implementation
//!
//! A background thread named "async-process" is lazily created on first use, which waits for
//! spawned child processes to exit and then calls the `wait()` syscall to clean up the "zombie"
//! processes. This is unlike the `process` API in the standard library, where dropping a running
//! `Child` leaks its resources.
//!
//! This crate uses [`async-io`] for async I/O on Unix-like systems and [`blocking`] for async I/O
//! on Windows.
//!
//! [`async-io`]: https://docs.rs/async-io
//! [`blocking`]: https://docs.rs/blocking
//!
//! # Examples
//!
//! Spawn a process and collect its output:
//!
//! ```no_run
//! # futures_lite::future::block_on(async {
//! use async_process::Command;
//!
//! let out = Command::new("echo").arg("hello").arg("world").output().await?;
//! assert_eq!(out.stdout, b"hello world\n");
//! # std::io::Result::Ok(()) });
//! ```
//!
//! Read the output line-by-line as it gets produced:
//!
//! ```no_run
//! # futures_lite::future::block_on(async {
//! use async_process::{Command, Stdio};
//! use futures_lite::{io::BufReader, prelude::*};
//!
//! let mut child = Command::new("find")
//!     .arg(".")
//!     .stdout(Stdio::piped())
//!     .spawn()?;
//!
//! let mut lines = BufReader::new(child.stdout.take().unwrap()).lines();
//!
//! while let Some(line) = lines.next().await {
//!     println!("{}", line?);
//! }
//! # std::io::Result::Ok(()) });
//! ```

#![warn(missing_docs, missing_debug_implementations, rust_2018_idioms)]
#![doc(
    html_favicon_url = "https://raw.githubusercontent.com/smol-rs/smol/master/assets/images/logo_fullsize_transparent.png"
)]
#![doc(
    html_logo_url = "https://raw.githubusercontent.com/smol-rs/smol/master/assets/images/logo_fullsize_transparent.png"
)]

use std::ffi::OsStr;
use std::fmt;
use std::path::Path;
use std::pin::Pin;
use std::sync::{Arc, Mutex};
use std::task::{Context, Poll};
use std::thread;

#[cfg(unix)]
use async_io::Async;
#[cfg(unix)]
use std::convert::{TryFrom, TryInto};
#[cfg(unix)]
use std::os::unix::io::{AsFd, AsRawFd, BorrowedFd, OwnedFd, RawFd};

#[cfg(windows)]
use blocking::Unblock;

use async_lock::OnceCell;
use event_listener::{Event, EventListener};
use futures_lite::{future, io, prelude::*};

#[doc(no_inline)]
pub use std::process::{ExitStatus, Output, Stdio};

#[cfg(unix)]
pub mod unix;
#[cfg(windows)]
pub mod windows;

mod sealed {
    pub trait Sealed {}
}

/// The zombie process reaper.
///
/// This structure reaps zombie processes and emits the `SIGCHLD` signal.
struct Reaper {
    /// An event delivered every time the SIGCHLD signal occurs.
    sigchld: Event,

    /// The list of zombie processes.
    zombies: Mutex<Vec<std::process::Child>>,

    /// The pipe that delivers signal notifications.
    pipe: Pipe,
}

impl Reaper {
    /// Get the singleton instance of the reaper.
    fn get() -> &'static Self {
        static REAPER: OnceCell<Reaper> = OnceCell::new();

        REAPER.get_or_init_blocking(|| {
            thread::Builder::new()
                .name("async-process".to_string())
                .spawn(|| REAPER.wait_blocking().reap())
                .expect("cannot spawn async-process thread");

            Reaper {
                sigchld: Event::new(),
                zombies: Mutex::new(Vec::new()),
                pipe: Pipe::new().expect("cannot create SIGCHLD pipe"),
            }
        })
    }

    /// Reap zombie processes forever.
    fn reap(&'static self) -> ! {
        loop {
            // Wait for the next SIGCHLD signal.
            self.pipe.wait();

            // Notify all listeners waiting on the SIGCHLD event.
            self.sigchld.notify(std::usize::MAX);

            // Reap zombie processes.
            let mut zombies = self.zombies.lock().unwrap();
            let mut i = 0;
            while i < zombies.len() {
                if let Ok(None) = zombies[i].try_wait() {
                    i += 1;
                } else {
                    zombies.swap_remove(i);
                }
            }
        }
    }

    /// Register a process with this reaper.
    fn register(&'static self, child: &std::process::Child) -> io::Result<()> {
        self.pipe.register(child)
    }
}

cfg_if::cfg_if! {
    if #[cfg(windows)] {
        use std::ffi::c_void;
        use std::os::windows::io::AsRawHandle;
        use std::sync::mpsc;

        use windows_sys::Win32::{
            Foundation::{BOOLEAN, HANDLE},
            System::Threading::{
                RegisterWaitForSingleObject, INFINITE, WT_EXECUTEINWAITTHREAD, WT_EXECUTEONLYONCE,
            },
        };

        /// Waits for the next SIGCHLD signal.
        struct Pipe {
            /// The sender channel for the SIGCHLD signal.
            sender: mpsc::SyncSender<()>,

            /// The receiver channel for the SIGCHLD signal.
            receiver: Mutex<mpsc::Receiver<()>>,
        }

        impl Pipe {
            /// Creates a new pipe.
            fn new() -> io::Result<Pipe> {
                let (sender, receiver) = mpsc::sync_channel(1);
                Ok(Pipe {
                    sender,
                    receiver: Mutex::new(receiver),
                })
            }

            /// Waits for the next SIGCHLD signal.
            fn wait(&self) {
                self.receiver.lock().unwrap().recv().ok();
            }

            /// Register a process object into this pipe.
            fn register(&self, child: &std::process::Child) -> io::Result<()> {
                // Called when a child exits.
                unsafe extern "system" fn callback(_: *mut c_void, _: BOOLEAN) {
                    Reaper::get().pipe.sender.try_send(()).ok();
                }

                // Register this child process to invoke `callback` on exit.
                let mut wait_object = 0;
                let ret = unsafe {
                    RegisterWaitForSingleObject(
                        &mut wait_object,
                        child.as_raw_handle() as HANDLE,
                        Some(callback),
                        std::ptr::null_mut(),
                        INFINITE,
                        WT_EXECUTEINWAITTHREAD | WT_EXECUTEONLYONCE,
                    )
                };

                if ret == 0 {
                    Err(io::Error::last_os_error())
                } else {
                    Ok(())
                }
            }
        }

        // Wraps a sync I/O type into an async I/O type.
        fn wrap<T>(io: T) -> io::Result<Unblock<T>> {
            Ok(Unblock::new(io))
        }
    } else if #[cfg(unix)] {
        use async_signal::{Signal, Signals};

        /// Waits for the next SIGCHLD signal.
        struct Pipe {
            /// The iterator over SIGCHLD signals.
            signals: Signals,
        }

        impl Pipe {
            /// Creates a new pipe.
            fn new() -> io::Result<Pipe> {
                Ok(Pipe {
                    signals: Signals::new(Some(Signal::Child))?,
                })
            }

            /// Waits for the next SIGCHLD signal.
            fn wait(&self) {
                async_io::block_on((&self.signals).next());
            }

            /// Register a process object into this pipe.
            fn register(&self, _child: &std::process::Child) -> io::Result<()> {
                Ok(())
            }
        }

        /// Wrap a file descriptor into a non-blocking I/O type.
        fn wrap<T: std::os::unix::io::AsRawFd>(io: T) -> io::Result<Async<T>> {
            Async::new(io)
        }
    }
}

/// A guard that can kill child processes, or push them into the zombie list.
struct ChildGuard {
    inner: Option<std::process::Child>,
    reap_on_drop: bool,
    kill_on_drop: bool,
}

impl ChildGuard {
    fn get_mut(&mut self) -> &mut std::process::Child {
        self.inner.as_mut().unwrap()
    }
}

// When the last reference to the child process is dropped, push it into the zombie list.
impl Drop for ChildGuard {
    fn drop(&mut self) {
        if self.kill_on_drop {
            self.get_mut().kill().ok();
        }
        if self.reap_on_drop {
            let mut zombies = Reaper::get().zombies.lock().unwrap();
            if let Ok(None) = self.get_mut().try_wait() {
                zombies.push(self.inner.take().unwrap());
            }
        }
    }
}

/// A spawned child process.
///
/// The process can be in running or exited state. Use [`status()`][`Child::status()`] or
/// [`output()`][`Child::output()`] to wait for it to exit.
///
/// If the [`Child`] is dropped, the process keeps running in the background.
///
/// # Examples
///
/// Spawn a process and wait for it to complete:
///
/// ```no_run
/// # futures_lite::future::block_on(async {
/// use async_process::Command;
///
/// Command::new("cp").arg("a.txt").arg("b.txt").status().await?;
/// # std::io::Result::Ok(()) });
/// ```
pub struct Child {
    /// The handle for writing to the child's standard input (stdin), if it has been captured.
    pub stdin: Option<ChildStdin>,

    /// The handle for reading from the child's standard output (stdout), if it has been captured.
    pub stdout: Option<ChildStdout>,

    /// The handle for reading from the child's standard error (stderr), if it has been captured.
    pub stderr: Option<ChildStderr>,

    /// The inner child process handle.
    child: Arc<Mutex<ChildGuard>>,
}

impl Child {
    /// Wraps the inner child process handle and registers it in the global process list.
    ///
    /// The "async-process" thread waits for processes in the global list and cleans up the
    /// resources when they exit.
    fn new(cmd: &mut Command) -> io::Result<Child> {
        // Make sure the reaper exists before we spawn the child process.
        let reaper = Reaper::get();
        let mut child = cmd.inner.spawn()?;

        // Convert sync I/O types into async I/O types.
        let stdin = child.stdin.take().map(wrap).transpose()?.map(ChildStdin);
        let stdout = child.stdout.take().map(wrap).transpose()?.map(ChildStdout);
        let stderr = child.stderr.take().map(wrap).transpose()?.map(ChildStderr);

        // Register the child process in the global list.
        reaper.register(&child)?;

        Ok(Child {
            stdin,
            stdout,
            stderr,
            child: Arc::new(Mutex::new(ChildGuard {
                inner: Some(child),
                reap_on_drop: cmd.reap_on_drop,
                kill_on_drop: cmd.kill_on_drop,
            })),
        })
    }

    /// Returns the OS-assigned process identifier associated with this child.
    ///
    /// # Examples
    ///
    /// ```no_run
    /// # futures_lite::future::block_on(async {
    /// use async_process::Command;
    ///
    /// let mut child = Command::new("ls").spawn()?;
    /// println!("id: {}", child.id());
    /// # std::io::Result::Ok(()) });
    /// ```
    pub fn id(&self) -> u32 {
        self.child.lock().unwrap().get_mut().id()
    }

    /// Forces the child process to exit.
    ///
    /// If the child has already exited, an [`InvalidInput`] error is returned.
    ///
    /// This is equivalent to sending a SIGKILL on Unix platforms.
    ///
    /// [`InvalidInput`]: `std::io::ErrorKind::InvalidInput`
    ///
    /// # Examples
    ///
    /// ```no_run
    /// # futures_lite::future::block_on(async {
    /// use async_process::Command;
    ///
    /// let mut child = Command::new("yes").spawn()?;
    /// child.kill()?;
    /// println!("exit status: {}", child.status().await?);
    /// # std::io::Result::Ok(()) });
    /// ```
    pub fn kill(&mut self) -> io::Result<()> {
        self.child.lock().unwrap().get_mut().kill()
    }

    /// Returns the exit status if the process has exited.
    ///
    /// Unlike [`status()`][`Child::status()`], this method will not drop the stdin handle.
    ///
    /// # Examples
    ///
    /// ```no_run
    /// # futures_lite::future::block_on(async {
    /// use async_process::Command;
    ///
    /// let mut child = Command::new("ls").spawn()?;
    ///
    /// match child.try_status()? {
    ///     None => println!("still running"),
    ///     Some(status) => println!("exited with: {}", status),
    /// }
    /// # std::io::Result::Ok(()) });
    /// ```
    pub fn try_status(&mut self) -> io::Result<Option<ExitStatus>> {
        self.child.lock().unwrap().get_mut().try_wait()
    }

    /// Drops the stdin handle and waits for the process to exit.
    ///
    /// Closing the stdin of the process helps avoid deadlocks. It ensures that the process does
    /// not block waiting for input from the parent process while the parent waits for the child to
    /// exit.
    ///
    /// # Examples
    ///
    /// ```no_run
    /// # futures_lite::future::block_on(async {
    /// use async_process::{Command, Stdio};
    ///
    /// let mut child = Command::new("cp")
    ///     .arg("a.txt")
    ///     .arg("b.txt")
    ///     .spawn()?;
    ///
    /// println!("exit status: {}", child.status().await?);
    /// # std::io::Result::Ok(()) });
    /// ```
    pub fn status(&mut self) -> impl Future<Output = io::Result<ExitStatus>> {
        self.stdin.take();
        let child = self.child.clone();

        async move {
            let listener = EventListener::new(&Reaper::get().sigchld);
            let mut listening = false;
            futures_lite::pin!(listener);

            loop {
                if let Some(status) = child.lock().unwrap().get_mut().try_wait()? {
                    return Ok(status);
                }

                if listening {
                    listener.as_mut().await;
                    listening = false;
                } else {
                    listener.as_mut().listen();
                    listening = true;
                }
            }
        }
    }

    /// Drops the stdin handle and collects the output of the process.
    ///
    /// Closing the stdin of the process helps avoid deadlocks. It ensures that the process does
    /// not block waiting for input from the parent process while the parent waits for the child to
    /// exit.
    ///
    /// In order to capture the output of the process, [`Command::stdout()`] and
    /// [`Command::stderr()`] must be configured with [`Stdio::piped()`].
    ///
    /// # Examples
    ///
    /// ```no_run
    /// # futures_lite::future::block_on(async {
    /// use async_process::{Command, Stdio};
    ///
    /// let child = Command::new("ls")
    ///     .stdout(Stdio::piped())
    ///     .stderr(Stdio::piped())
    ///     .spawn()?;
    ///
    /// let out = child.output().await?;
    /// # std::io::Result::Ok(()) });
    /// ```
    pub fn output(mut self) -> impl Future<Output = io::Result<Output>> {
        // A future that waits for the exit status.
        let status = self.status();

        // A future that collects stdout.
        let stdout = self.stdout.take();
        let stdout = async move {
            let mut v = Vec::new();
            if let Some(mut s) = stdout {
                s.read_to_end(&mut v).await?;
            }
            io::Result::Ok(v)
        };

        // A future that collects stderr.
        let stderr = self.stderr.take();
        let stderr = async move {
            let mut v = Vec::new();
            if let Some(mut s) = stderr {
                s.read_to_end(&mut v).await?;
            }
            io::Result::Ok(v)
        };

        async move {
            let (stdout, stderr) = future::try_zip(stdout, stderr).await?;
            let status = status.await?;
            Ok(Output {
                status,
                stdout,
                stderr,
            })
        }
    }
}

impl fmt::Debug for Child {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Child")
            .field("stdin", &self.stdin)
            .field("stdout", &self.stdout)
            .field("stderr", &self.stderr)
            .finish()
    }
}

/// A handle to a child process's standard input (stdin).
///
/// When a [`ChildStdin`] is dropped, the underlying handle gets clossed. If the child process was
/// previously blocked on input, it becomes unblocked after dropping.
#[derive(Debug)]
pub struct ChildStdin(
    #[cfg(windows)] Unblock<std::process::ChildStdin>,
    #[cfg(unix)] Async<std::process::ChildStdin>,
);

impl ChildStdin {
    /// Convert async_process::ChildStdin into std::process::Stdio.
    ///
    /// You can use it to associate to the next process.
    ///
    /// # Examples
    ///
    /// ```no_run
    /// # futures_lite::future::block_on(async {
    /// use async_process::Command;
    /// use std::process::Stdio;
    ///
    /// let mut ls_child = Command::new("ls").stdin(Stdio::piped()).spawn()?;
    /// let stdio:Stdio = ls_child.stdin.take().unwrap().into_stdio().await?;
    ///
    /// let mut echo_child = Command::new("echo").arg("./").stdout(stdio).spawn()?;
    ///
    /// # std::io::Result::Ok(()) });
    /// ```
    pub async fn into_stdio(self) -> io::Result<std::process::Stdio> {
        cfg_if::cfg_if! {
            if #[cfg(windows)] {
                Ok(self.0.into_inner().await.into())
            } else if #[cfg(unix)] {
                let child_stdin = self.0.into_inner()?;
                blocking_fd(rustix::fd::AsFd::as_fd(&child_stdin))?;
                Ok(child_stdin.into())
            }
        }
    }
}

impl io::AsyncWrite for ChildStdin {
    fn poll_write(
        mut self: Pin<&mut Self>,
        cx: &mut Context<'_>,
        buf: &[u8],
    ) -> Poll<io::Result<usize>> {
        Pin::new(&mut self.0).poll_write(cx, buf)
    }

    fn poll_flush(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<io::Result<()>> {
        Pin::new(&mut self.0).poll_flush(cx)
    }

    fn poll_close(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<io::Result<()>> {
        Pin::new(&mut self.0).poll_close(cx)
    }
}

#[cfg(unix)]
impl AsRawFd for ChildStdin {
    fn as_raw_fd(&self) -> RawFd {
        self.0.as_raw_fd()
    }
}

#[cfg(unix)]
impl AsFd for ChildStdin {
    fn as_fd(&self) -> BorrowedFd<'_> {
        self.0.as_fd()
    }
}

#[cfg(unix)]
impl TryFrom<ChildStdin> for OwnedFd {
    type Error = io::Error;

    fn try_from(value: ChildStdin) -> Result<Self, Self::Error> {
        value.0.try_into()
    }
}

// TODO(notgull): Add mirroring AsRawHandle impls for all of the child handles
//
// at the moment this is pretty hard to do because of how they're wrapped in
// Unblock, meaning that we can't always access the underlying handle. async-fs
// gets around this by putting the handle in an Arc, but there's still some decision
// to be made about how to handle this (no pun intended)

/// A handle to a child process's standard output (stdout).
///
/// When a [`ChildStdout`] is dropped, the underlying handle gets closed.
#[derive(Debug)]
pub struct ChildStdout(
    #[cfg(windows)] Unblock<std::process::ChildStdout>,
    #[cfg(unix)] Async<std::process::ChildStdout>,
);

impl ChildStdout {
    /// Convert async_process::ChildStdout into std::process::Stdio.
    ///
    /// You can use it to associate to the next process.
    ///
    /// # Examples
    ///
    /// ```no_run
    /// # futures_lite::future::block_on(async {
    /// use async_process::Command;
    /// use std::process::Stdio;
    /// use std::io::Read;
    /// use futures_lite::AsyncReadExt;
    ///
    /// let mut ls_child = Command::new("ls").stdout(Stdio::piped()).spawn()?;
    /// let stdio:Stdio = ls_child.stdout.take().unwrap().into_stdio().await?;
    ///
    /// let mut echo_child = Command::new("echo").stdin(stdio).stdout(Stdio::piped()).spawn()?;
    /// let mut buf = vec![];
    /// echo_child.stdout.take().unwrap().read(&mut buf).await;
    /// # std::io::Result::Ok(()) });
    /// ```
    pub async fn into_stdio(self) -> io::Result<std::process::Stdio> {
        cfg_if::cfg_if! {
            if #[cfg(windows)] {
                Ok(self.0.into_inner().await.into())
            } else if #[cfg(unix)] {
                let child_stdout = self.0.into_inner()?;
                blocking_fd(rustix::fd::AsFd::as_fd(&child_stdout))?;
                Ok(child_stdout.into())
            }
        }
    }
}

impl io::AsyncRead for ChildStdout {
    fn poll_read(
        mut self: Pin<&mut Self>,
        cx: &mut Context<'_>,
        buf: &mut [u8],
    ) -> Poll<io::Result<usize>> {
        Pin::new(&mut self.0).poll_read(cx, buf)
    }
}

#[cfg(unix)]
impl AsRawFd for ChildStdout {
    fn as_raw_fd(&self) -> RawFd {
        self.0.as_raw_fd()
    }
}

#[cfg(unix)]
impl AsFd for ChildStdout {
    fn as_fd(&self) -> BorrowedFd<'_> {
        self.0.as_fd()
    }
}

#[cfg(unix)]
impl TryFrom<ChildStdout> for OwnedFd {
    type Error = io::Error;

    fn try_from(value: ChildStdout) -> Result<Self, Self::Error> {
        value.0.try_into()
    }
}

/// A handle to a child process's standard error (stderr).
///
/// When a [`ChildStderr`] is dropped, the underlying handle gets closed.
#[derive(Debug)]
pub struct ChildStderr(
    #[cfg(windows)] Unblock<std::process::ChildStderr>,
    #[cfg(unix)] Async<std::process::ChildStderr>,
);

impl ChildStderr {
    /// Convert async_process::ChildStderr into std::process::Stdio.
    ///
    /// You can use it to associate to the next process.
    ///
    /// # Examples
    ///
    /// ```no_run
    /// # futures_lite::future::block_on(async {
    /// use async_process::Command;
    /// use std::process::Stdio;
    ///
    /// let mut ls_child = Command::new("ls").arg("x").stderr(Stdio::piped()).spawn()?;
    /// let stdio:Stdio = ls_child.stderr.take().unwrap().into_stdio().await?;
    ///
    /// let mut echo_child = Command::new("echo").stdin(stdio).spawn()?;
    /// # std::io::Result::Ok(()) });
    /// ```
    pub async fn into_stdio(self) -> io::Result<std::process::Stdio> {
        cfg_if::cfg_if! {
            if #[cfg(windows)] {
                Ok(self.0.into_inner().await.into())
            } else if #[cfg(unix)] {
                let child_stderr = self.0.into_inner()?;
                blocking_fd(rustix::fd::AsFd::as_fd(&child_stderr))?;
                Ok(child_stderr.into())
            }
        }
    }
}

impl io::AsyncRead for ChildStderr {
    fn poll_read(
        mut self: Pin<&mut Self>,
        cx: &mut Context<'_>,
        buf: &mut [u8],
    ) -> Poll<io::Result<usize>> {
        Pin::new(&mut self.0).poll_read(cx, buf)
    }
}

#[cfg(unix)]
impl AsRawFd for ChildStderr {
    fn as_raw_fd(&self) -> RawFd {
        self.0.as_raw_fd()
    }
}

#[cfg(unix)]
impl AsFd for ChildStderr {
    fn as_fd(&self) -> BorrowedFd<'_> {
        self.0.as_fd()
    }
}

#[cfg(unix)]
impl TryFrom<ChildStderr> for OwnedFd {
    type Error = io::Error;

    fn try_from(value: ChildStderr) -> Result<Self, Self::Error> {
        value.0.try_into()
    }
}

/// A builder for spawning processes.
///
/// # Examples
///
/// ```no_run
/// # futures_lite::future::block_on(async {
/// use async_process::Command;
///
/// let output = if cfg!(target_os = "windows") {
///     Command::new("cmd").args(&["/C", "echo hello"]).output().await?
/// } else {
///     Command::new("sh").arg("-c").arg("echo hello").output().await?
/// };
/// # std::io::Result::Ok(()) });
/// ```
pub struct Command {
    inner: std::process::Command,
    stdin: bool,
    stdout: bool,
    stderr: bool,
    reap_on_drop: bool,
    kill_on_drop: bool,
}

impl Command {
    /// Constructs a new [`Command`] for launching `program`.
    ///
    /// The initial configuration (the working directory and environment variables) is inherited
    /// from the current process.
    ///
    /// # Examples
    ///
    /// ```
    /// use async_process::Command;
    ///
    /// let mut cmd = Command::new("ls");
    /// ```
    pub fn new<S: AsRef<OsStr>>(program: S) -> Command {
        Self::from(std::process::Command::new(program))
    }

    /// Adds a single argument to pass to the program.
    ///
    /// # Examples
    ///
    /// ```
    /// use async_process::Command;
    ///
    /// let mut cmd = Command::new("echo");
    /// cmd.arg("hello");
    /// cmd.arg("world");
    /// ```
    pub fn arg<S: AsRef<OsStr>>(&mut self, arg: S) -> &mut Command {
        self.inner.arg(arg);
        self
    }

    /// Adds multiple arguments to pass to the program.
    ///
    /// # Examples
    ///
    /// ```
    /// use async_process::Command;
    ///
    /// let mut cmd = Command::new("echo");
    /// cmd.args(&["hello", "world"]);
    /// ```
    pub fn args<I, S>(&mut self, args: I) -> &mut Command
    where
        I: IntoIterator<Item = S>,
        S: AsRef<OsStr>,
    {
        self.inner.args(args);
        self
    }

    /// Configures an environment variable for the new process.
    ///
    /// Note that environment variable names are case-insensitive (but case-preserving) on Windows,
    /// and case-sensitive on all other platforms.
    ///
    /// # Examples
    ///
    /// ```
    /// use async_process::Command;
    ///
    /// let mut cmd = Command::new("ls");
    /// cmd.env("PATH", "/bin");
    /// ```
    pub fn env<K, V>(&mut self, key: K, val: V) -> &mut Command
    where
        K: AsRef<OsStr>,
        V: AsRef<OsStr>,
    {
        self.inner.env(key, val);
        self
    }

    /// Configures multiple environment variables for the new process.
    ///
    /// Note that environment variable names are case-insensitive (but case-preserving) on Windows,
    /// and case-sensitive on all other platforms.
    ///
    /// # Examples
    ///
    /// ```
    /// use async_process::Command;
    ///
    /// let mut cmd = Command::new("ls");
    /// cmd.envs(vec![("PATH", "/bin"), ("TERM", "xterm-256color")]);
    /// ```
    pub fn envs<I, K, V>(&mut self, vars: I) -> &mut Command
    where
        I: IntoIterator<Item = (K, V)>,
        K: AsRef<OsStr>,
        V: AsRef<OsStr>,
    {
        self.inner.envs(vars);
        self
    }

    /// Removes an environment variable mapping.
    ///
    /// # Examples
    ///
    /// ```
    /// use async_process::Command;
    ///
    /// let mut cmd = Command::new("ls");
    /// cmd.env_remove("PATH");
    /// ```
    pub fn env_remove<K: AsRef<OsStr>>(&mut self, key: K) -> &mut Command {
        self.inner.env_remove(key);
        self
    }

    /// Removes all environment variable mappings.
    ///
    /// # Examples
    ///
    /// ```
    /// use async_process::Command;
    ///
    /// let mut cmd = Command::new("ls");
    /// cmd.env_clear();
    /// ```
    pub fn env_clear(&mut self) -> &mut Command {
        self.inner.env_clear();
        self
    }

    /// Configures the working directory for the new process.
    ///
    /// # Examples
    ///
    /// ```
    /// use async_process::Command;
    ///
    /// let mut cmd = Command::new("ls");
    /// cmd.current_dir("/");
    /// ```
    pub fn current_dir<P: AsRef<Path>>(&mut self, dir: P) -> &mut Command {
        self.inner.current_dir(dir);
        self
    }

    /// Configures the standard input (stdin) for the new process.
    ///
    /// # Examples
    ///
    /// ```
    /// use async_process::{Command, Stdio};
    ///
    /// let mut cmd = Command::new("cat");
    /// cmd.stdin(Stdio::null());
    /// ```
    pub fn stdin<T: Into<Stdio>>(&mut self, cfg: T) -> &mut Command {
        self.stdin = true;
        self.inner.stdin(cfg);
        self
    }

    /// Configures the standard output (stdout) for the new process.
    ///
    /// # Examples
    ///
    /// ```
    /// use async_process::{Command, Stdio};
    ///
    /// let mut cmd = Command::new("ls");
    /// cmd.stdout(Stdio::piped());
    /// ```
    pub fn stdout<T: Into<Stdio>>(&mut self, cfg: T) -> &mut Command {
        self.stdout = true;
        self.inner.stdout(cfg);
        self
    }

    /// Configures the standard error (stderr) for the new process.
    ///
    /// # Examples
    ///
    /// ```
    /// use async_process::{Command, Stdio};
    ///
    /// let mut cmd = Command::new("ls");
    /// cmd.stderr(Stdio::piped());
    /// ```
    pub fn stderr<T: Into<Stdio>>(&mut self, cfg: T) -> &mut Command {
        self.stderr = true;
        self.inner.stderr(cfg);
        self
    }

    /// Configures whether to reap the zombie process when [`Child`] is dropped.
    ///
    /// When the process finishes, it becomes a "zombie" and some resources associated with it
    /// remain until [`Child::try_status()`], [`Child::status()`], or [`Child::output()`] collects
    /// its exit code.
    ///
    /// If its exit code is never collected, the resources may leak forever. This crate has a
    /// background thread named "async-process" that collects such "zombie" processes and then
    /// "reaps" them, thus preventing the resource leaks.
    ///
    /// The default value of this option is `true`.
    ///
    /// # Examples
    ///
    /// ```
    /// use async_process::{Command, Stdio};
    ///
    /// let mut cmd = Command::new("cat");
    /// cmd.reap_on_drop(false);
    /// ```
    pub fn reap_on_drop(&mut self, reap_on_drop: bool) -> &mut Command {
        self.reap_on_drop = reap_on_drop;
        self
    }

    /// Configures whether to kill the process when [`Child`] is dropped.
    ///
    /// The default value of this option is `false`.
    ///
    /// # Examples
    ///
    /// ```
    /// use async_process::{Command, Stdio};
    ///
    /// let mut cmd = Command::new("cat");
    /// cmd.kill_on_drop(true);
    /// ```
    pub fn kill_on_drop(&mut self, kill_on_drop: bool) -> &mut Command {
        self.kill_on_drop = kill_on_drop;
        self
    }

    /// Executes the command and returns the [`Child`] handle to it.
    ///
    /// If not configured, stdin, stdout and stderr will be set to [`Stdio::inherit()`].
    ///
    /// # Examples
    ///
    /// ```no_run
    /// # futures_lite::future::block_on(async {
    /// use async_process::Command;
    ///
    /// let child = Command::new("ls").spawn()?;
    /// # std::io::Result::Ok(()) });
    /// ```
    pub fn spawn(&mut self) -> io::Result<Child> {
        if !self.stdin {
            self.inner.stdin(Stdio::inherit());
        }
        if !self.stdout {
            self.inner.stdout(Stdio::inherit());
        }
        if !self.stderr {
            self.inner.stderr(Stdio::inherit());
        }

        Child::new(self)
    }

    /// Executes the command, waits for it to exit, and returns the exit status.
    ///
    /// If not configured, stdin, stdout and stderr will be set to [`Stdio::inherit()`].
    ///
    /// # Examples
    ///
    /// ```no_run
    /// # futures_lite::future::block_on(async {
    /// use async_process::Command;
    ///
    /// let status = Command::new("cp")
    ///     .arg("a.txt")
    ///     .arg("b.txt")
    ///     .status()
    ///     .await?;
    /// # std::io::Result::Ok(()) });
    /// ```
    pub fn status(&mut self) -> impl Future<Output = io::Result<ExitStatus>> {
        let child = self.spawn();
        async { child?.status().await }
    }

    /// Executes the command and collects its output.
    ///
    /// If not configured, stdin will be set to [`Stdio::null()`], and stdout and stderr will be
    /// set to [`Stdio::piped()`].
    ///
    /// # Examples
    ///
    /// ```no_run
    /// # futures_lite::future::block_on(async {
    /// use async_process::Command;
    ///
    /// let output = Command::new("cat")
    ///     .arg("a.txt")
    ///     .output()
    ///     .await?;
    /// # std::io::Result::Ok(()) });
    /// ```
    pub fn output(&mut self) -> impl Future<Output = io::Result<Output>> {
        if !self.stdin {
            self.inner.stdin(Stdio::null());
        }
        if !self.stdout {
            self.inner.stdout(Stdio::piped());
        }
        if !self.stderr {
            self.inner.stderr(Stdio::piped());
        }

        let child = Child::new(self);
        async { child?.output().await }
    }
}

impl From<std::process::Command> for Command {
    fn from(inner: std::process::Command) -> Self {
        Self {
            inner,
            stdin: false,
            stdout: false,
            stderr: false,
            reap_on_drop: true,
            kill_on_drop: false,
        }
    }
}

impl fmt::Debug for Command {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if f.alternate() {
            f.debug_struct("Command")
                .field("inner", &self.inner)
                .field("stdin", &self.stdin)
                .field("stdout", &self.stdout)
                .field("stderr", &self.stderr)
                .field("reap_on_drop", &self.reap_on_drop)
                .field("kill_on_drop", &self.kill_on_drop)
                .finish()
        } else {
            // Stdlib outputs command-line in Debug for Command. This does the
            // same, if not in "alternate" (long pretty-printed) mode.
            // This is useful for logs, for example.
            fmt::Debug::fmt(&self.inner, f)
        }
    }
}

/// Moves `Fd` out of non-blocking mode.
#[cfg(unix)]
fn blocking_fd(fd: rustix::fd::BorrowedFd<'_>) -> io::Result<()> {
    cfg_if::cfg_if! {
        // ioctl(FIONBIO) sets the flag atomically, but we use this only on Linux
        // for now, as with the standard library, because it seems to behave
        // differently depending on the platform.
        // https://github.com/rust-lang/rust/commit/efeb42be2837842d1beb47b51bb693c7474aba3d
        // https://github.com/libuv/libuv/blob/e9d91fccfc3e5ff772d5da90e1c4a24061198ca0/src/unix/poll.c#L78-L80
        // https://github.com/tokio-rs/mio/commit/0db49f6d5caf54b12176821363d154384357e70a
        if #[cfg(target_os = "linux")] {
            rustix::io::ioctl_fionbio(fd, false)?;
        } else {
            let previous = rustix::fs::fcntl_getfl(fd)?;
            let new = previous & !rustix::fs::OFlags::NONBLOCK;
            if new != previous {
                rustix::fs::fcntl_setfl(fd, new)?;
            }
        }
    }
    Ok(())
}

#[cfg(unix)]
mod test {

    #[test]
    fn test_into_inner() {
        futures_lite::future::block_on(async {
            use crate::Command;

            use std::io::Result;
            use std::process::Stdio;
            use std::str::from_utf8;

            use futures_lite::AsyncReadExt;

            let mut ls_child = Command::new("cat")
                .arg("Cargo.toml")
                .stdout(Stdio::piped())
                .spawn()?;

            let stdio: Stdio = ls_child.stdout.take().unwrap().into_stdio().await?;

            let mut echo_child = Command::new("grep")
                .arg("async")
                .stdin(stdio)
                .stdout(Stdio::piped())
                .spawn()?;

            let mut buf = vec![];
            let mut stdout = echo_child.stdout.take().unwrap();

            stdout.read_to_end(&mut buf).await?;
            dbg!(from_utf8(&buf).unwrap_or(""));

            Result::Ok(())
        })
        .unwrap();
    }
}
