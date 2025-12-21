#![deny(rust_2018_idioms)]
#![doc(
    html_logo_url = "https://storage.googleapis.com/fdo-gitlab-uploads/project/avatar/3213/zbus-logomark.png"
)]
#![doc = include_str!("../README.md")]
#![doc(test(attr(
    warn(unused),
    deny(warnings),
    // W/o this, we seem to get some bogus warning about `extern crate zbus`.
    allow(unused_extern_crates),
)))]

#[cfg(doctest)]
mod doctests {
    // Book markdown checks
    doc_comment::doctest!("../../book/src/client.md");
    doc_comment::doctest!("../../book/src/concepts.md");
    doc_comment::doctest!("../../book/src/connection.md");
    doc_comment::doctest!("../../book/src/contributors.md");
    doc_comment::doctest!("../../book/src/introduction.md");
    doc_comment::doctest!("../../book/src/server.md");
    doc_comment::doctest!("../../book/src/blocking.md");
    doc_comment::doctest!("../../book/src/faq.md");
}

#[cfg(all(not(feature = "async-io"), not(feature = "tokio")))]
mod error_message {
    #[cfg(windows)]
    compile_error!("Either \"async-io\" (default) or \"tokio\" must be enabled. On Windows \"async-io\" is (currently) required for UNIX socket support");

    #[cfg(not(windows))]
    compile_error!("Either \"async-io\" (default) or \"tokio\" must be enabled.");
}

#[cfg(windows)]
mod win32;

mod dbus_error;
pub use dbus_error::*;

mod error;
pub use error::*;

mod address;
pub use address::*;

mod guid;
pub use guid::*;

mod message;
pub use message::*;

mod message_builder;
pub use message_builder::*;

mod message_header;
pub use message_header::*;

mod message_field;
pub use message_field::*;

mod message_fields;
pub use message_fields::*;

mod handshake;
pub use handshake::AuthMechanism;
pub(crate) use handshake::*;

mod connection;
pub use connection::*;
mod connection_builder;
pub use connection_builder::*;
mod message_stream;
pub use message_stream::*;
mod object_server;
pub use object_server::*;
mod proxy;
pub use proxy::*;
mod proxy_builder;
pub use proxy_builder::*;
mod signal_context;
pub use signal_context::*;
mod interface;
pub use interface::*;
mod abstractions;
pub use abstractions::*;
mod match_rule;
pub use match_rule::*;
mod match_rule_builder;
pub use match_rule_builder::*;
mod socket_reader;

mod utils;
pub use utils::*;

#[macro_use]
pub mod fdo;

mod raw;
pub use raw::Socket;

pub mod blocking;

#[cfg(feature = "xml")]
pub mod xml;

#[cfg(feature = "quick-xml")]
pub mod quick_xml;

pub use zbus_macros::{dbus_interface, dbus_proxy, DBusError};

// Required for the macros to function within this crate.
extern crate self as zbus;

// Macro support module, not part of the public API.
#[doc(hidden)]
pub mod export {
    pub use async_trait;
    pub use futures_core;
    pub use futures_util;
    pub use ordered_stream;
    pub use serde;
    pub use static_assertions;
}

pub use zbus_names as names;
pub use zvariant;

#[cfg(unix)]
use zvariant::OwnedFd;

#[cfg(test)]
mod tests {
    use std::{
        collections::HashMap,
        convert::{TryFrom, TryInto},
        sync::{mpsc::channel, Arc, Condvar, Mutex},
    };
    #[cfg(unix)]
    use std::{
        fs::File,
        os::unix::io::{AsRawFd, FromRawFd},
    };

    use crate::utils::block_on;
    use enumflags2::BitFlags;
    use ntest::timeout;
    use test_log::test;
    use tracing::{debug, instrument, trace};

    use zbus_names::UniqueName;
    #[cfg(unix)]
    use zvariant::Fd;
    use zvariant::{OwnedObjectPath, OwnedValue, Type};

    use crate::{
        blocking::{self, MessageIterator},
        fdo::{RequestNameFlags, RequestNameReply},
        Connection, Message, MessageFlags, Result, SignalContext,
    };

    fn is_gdbus_test() -> bool {
        std::env::var_os("ZBUS_GDBUS_TEST").is_some()
    }

    #[test]
    fn msg() {
        let mut m = Message::method(
            None::<()>,
            Some("org.freedesktop.DBus"),
            "/org/freedesktop/DBus",
            Some("org.freedesktop.DBus.Peer"),
            "GetMachineId",
            &(),
        )
        .unwrap();
        assert_eq!(m.path().unwrap(), "/org/freedesktop/DBus");
        assert_eq!(m.interface().unwrap(), "org.freedesktop.DBus.Peer");
        assert_eq!(m.member().unwrap(), "GetMachineId");
        m.modify_primary_header(|primary| {
            primary.set_flags(BitFlags::from(MessageFlags::NoAutoStart));
            primary.serial_num_or_init(|| 11);

            Ok(())
        })
        .unwrap();
        let primary = m.primary_header();
        assert!(*primary.serial_num().unwrap() == 11);
        assert!(primary.flags() == MessageFlags::NoAutoStart);
    }

    #[test]
    #[timeout(15000)]
    #[instrument]
    fn basic_connection() {
        let connection = blocking::Connection::session()
            .map_err(|e| {
                debug!("error: {}", e);

                e
            })
            .unwrap();
        // Hello method is already called during connection creation so subsequent calls are
        // expected to fail but only with a D-Bus error.
        match connection.call_method(
            Some("org.freedesktop.DBus"),
            "/org/freedesktop/DBus",
            Some("org.freedesktop.DBus"),
            "Hello",
            &(),
        ) {
            Err(crate::Error::MethodError(_, _, _)) => (),
            Err(e) => panic!("{}", e),

            // GDBus allows the method to be called multiple times
            Ok(_) if is_gdbus_test() => (),

            _ => panic!(),
        };
    }

    #[test]
    #[timeout(15000)]
    fn basic_connection_async() {
        block_on(test_basic_connection()).unwrap();
    }

    async fn test_basic_connection() -> Result<()> {
        let connection = Connection::session().await?;

        match connection
            .call_method(
                Some("org.freedesktop.DBus"),
                "/org/freedesktop/DBus",
                Some("org.freedesktop.DBus"),
                "Hello",
                &(),
            )
            .await
        {
            Err(crate::Error::MethodError(_, _, _)) => (),
            Err(e) => panic!("{}", e),

            // GDBus allows the method to be called multiple times
            Ok(_) if is_gdbus_test() => (),

            _ => panic!(),
        };

        Ok(())
    }

    #[cfg(all(unix, not(target_os = "macos")))]
    #[test]
    #[timeout(15000)]
    fn fdpass_systemd() {
        let connection = blocking::Connection::system().unwrap();

        let reply = connection
            .call_method(
                Some("org.freedesktop.systemd1"),
                "/org/freedesktop/systemd1",
                Some("org.freedesktop.systemd1.Manager"),
                "DumpByFileDescriptor",
                &(),
            )
            .unwrap();

        assert!(reply
            .body_signature()
            .map(|s| s == <Fd>::signature())
            .unwrap());

        let fd: Fd = reply.body().unwrap();
        let _fds = reply.take_fds();
        assert!(fd.as_raw_fd() >= 0);
        let f = unsafe { File::from_raw_fd(fd.as_raw_fd()) };
        f.metadata().unwrap();
    }

    #[test]
    #[instrument]
    #[timeout(15000)]
    fn freedesktop_api() {
        let connection = blocking::Connection::session()
            .map_err(|e| {
                debug!("error: {}", e);

                e
            })
            .unwrap();

        let reply = connection
            .call_method(
                Some("org.freedesktop.DBus"),
                "/org/freedesktop/DBus",
                Some("org.freedesktop.DBus"),
                "RequestName",
                &(
                    "org.freedesktop.zbus.sync",
                    BitFlags::from(RequestNameFlags::ReplaceExisting),
                ),
            )
            .unwrap();

        assert!(reply.body_signature().map(|s| s == "u").unwrap());
        let reply: RequestNameReply = reply.body().unwrap();
        assert_eq!(reply, RequestNameReply::PrimaryOwner);

        let reply = connection
            .call_method(
                Some("org.freedesktop.DBus"),
                "/org/freedesktop/DBus",
                Some("org.freedesktop.DBus"),
                "GetId",
                &(),
            )
            .unwrap();

        assert!(reply
            .body_signature()
            .map(|s| s == <&str>::signature())
            .unwrap());
        let id: &str = reply.body().unwrap();
        debug!("Unique ID of the bus: {}", id);

        let reply = connection
            .call_method(
                Some("org.freedesktop.DBus"),
                "/org/freedesktop/DBus",
                Some("org.freedesktop.DBus"),
                "NameHasOwner",
                &"org.freedesktop.zbus.sync",
            )
            .unwrap();

        assert!(reply
            .body_signature()
            .map(|s| s == bool::signature())
            .unwrap());
        assert!(reply.body::<bool>().unwrap());

        let reply = connection
            .call_method(
                Some("org.freedesktop.DBus"),
                "/org/freedesktop/DBus",
                Some("org.freedesktop.DBus"),
                "GetNameOwner",
                &"org.freedesktop.zbus.sync",
            )
            .unwrap();

        assert!(reply
            .body_signature()
            .map(|s| s == <&str>::signature())
            .unwrap());
        assert_eq!(
            reply.body::<UniqueName<'_>>().unwrap(),
            *connection.unique_name().unwrap(),
        );

        // GDBus doesn't provide this method
        if is_gdbus_test() {
            return;
        }

        let reply = connection
            .call_method(
                Some("org.freedesktop.DBus"),
                "/org/freedesktop/DBus",
                Some("org.freedesktop.DBus"),
                "GetConnectionCredentials",
                &"org.freedesktop.DBus",
            )
            .unwrap();

        assert!(reply.body_signature().map(|s| s == "a{sv}").unwrap());
        let hashmap: HashMap<&str, OwnedValue> = reply.body().unwrap();

        let pid: u32 = (&hashmap["ProcessID"]).try_into().unwrap();
        debug!("DBus bus PID: {}", pid);

        #[cfg(unix)]
        {
            let uid: u32 = (&hashmap["UnixUserID"]).try_into().unwrap();
            debug!("DBus bus UID: {}", uid);
        }
    }

    #[test]
    #[timeout(15000)]
    fn freedesktop_api_async() {
        block_on(test_freedesktop_api()).unwrap();
    }

    #[instrument]
    async fn test_freedesktop_api() -> Result<()> {
        let connection = Connection::session().await?;

        let reply = connection
            .call_method(
                Some("org.freedesktop.DBus"),
                "/org/freedesktop/DBus",
                Some("org.freedesktop.DBus"),
                "RequestName",
                &(
                    "org.freedesktop.zbus.async",
                    BitFlags::from(RequestNameFlags::ReplaceExisting),
                ),
            )
            .await
            .unwrap();

        assert!(reply.body_signature().map(|s| s == "u").unwrap());
        let reply: RequestNameReply = reply.body().unwrap();
        assert_eq!(reply, RequestNameReply::PrimaryOwner);

        let reply = connection
            .call_method(
                Some("org.freedesktop.DBus"),
                "/org/freedesktop/DBus",
                Some("org.freedesktop.DBus"),
                "GetId",
                &(),
            )
            .await
            .unwrap();

        assert!(reply
            .body_signature()
            .map(|s| s == <&str>::signature())
            .unwrap());
        let id: &str = reply.body().unwrap();
        debug!("Unique ID of the bus: {}", id);

        let reply = connection
            .call_method(
                Some("org.freedesktop.DBus"),
                "/org/freedesktop/DBus",
                Some("org.freedesktop.DBus"),
                "NameHasOwner",
                &"org.freedesktop.zbus.async",
            )
            .await
            .unwrap();

        assert!(reply
            .body_signature()
            .map(|s| s == bool::signature())
            .unwrap());
        assert!(reply.body::<bool>().unwrap());

        let reply = connection
            .call_method(
                Some("org.freedesktop.DBus"),
                "/org/freedesktop/DBus",
                Some("org.freedesktop.DBus"),
                "GetNameOwner",
                &"org.freedesktop.zbus.async",
            )
            .await
            .unwrap();

        assert!(reply
            .body_signature()
            .map(|s| s == <&str>::signature())
            .unwrap());
        assert_eq!(
            reply.body::<UniqueName<'_>>().unwrap(),
            *connection.unique_name().unwrap(),
        );

        // GDBus doesn't provide this method
        if is_gdbus_test() {
            return Ok(());
        }

        let reply = connection
            .call_method(
                Some("org.freedesktop.DBus"),
                "/org/freedesktop/DBus",
                Some("org.freedesktop.DBus"),
                "GetConnectionCredentials",
                &"org.freedesktop.DBus",
            )
            .await
            .unwrap();

        assert!(reply.body_signature().map(|s| s == "a{sv}").unwrap());
        let hashmap: HashMap<&str, OwnedValue> = reply.body().unwrap();

        let pid: u32 = (&hashmap["ProcessID"]).try_into().unwrap();
        debug!("DBus bus PID: {}", pid);

        #[cfg(unix)]
        {
            let uid: u32 = (&hashmap["UnixUserID"]).try_into().unwrap();
            debug!("DBus bus UID: {}", uid);
        }

        Ok(())
    }

    #[test]
    #[timeout(15000)]
    fn issue_68() {
        // Tests the fix for https://github.com/dbus2/zbus/issues/68
        //
        // While this is not an exact reproduction of the issue 68, the underlying problem it
        // produces is exactly the same: `Connection::call_method` dropping all incoming messages
        // while waiting for the reply to the method call.
        let conn = blocking::Connection::session().unwrap();
        let stream = MessageIterator::from(&conn);

        // Send a message as client before service starts to process messages
        let client_conn = blocking::Connection::session().unwrap();
        let destination = conn.unique_name().map(UniqueName::<'_>::from);
        let msg = Message::method(
            None::<()>,
            destination,
            "/org/freedesktop/Issue68",
            Some("org.freedesktop.Issue68"),
            "Ping",
            &(),
        )
        .unwrap();
        let serial = client_conn.send_message(msg).unwrap();

        crate::blocking::fdo::DBusProxy::new(&conn)
            .unwrap()
            .get_id()
            .unwrap();

        for m in stream {
            let msg = m.unwrap();

            if *msg.primary_header().serial_num().unwrap() == serial {
                break;
            }
        }
    }

    #[test]
    #[timeout(15000)]
    fn issue104() {
        // Tests the fix for https://github.com/dbus2/zbus/issues/104
        //
        // The issue is caused by `dbus_proxy` macro adding `()` around the return value of methods
        // with multiple out arguments, ending up with double parenthesis around the signature of
        // the return type and zbus only removing the outer `()` only and then it not matching the
        // signature we receive on the reply message.
        use zvariant::{ObjectPath, Value};

        struct Secret;
        #[super::dbus_interface(name = "org.freedesktop.Secret.Service")]
        impl Secret {
            fn open_session(
                &self,
                _algorithm: &str,
                input: Value<'_>,
            ) -> zbus::fdo::Result<(OwnedValue, OwnedObjectPath)> {
                Ok((
                    OwnedValue::from(input),
                    ObjectPath::try_from("/org/freedesktop/secrets/Blah")
                        .unwrap()
                        .into(),
                ))
            }
        }

        let secret = Secret;
        let conn = blocking::ConnectionBuilder::session()
            .unwrap()
            .serve_at("/org/freedesktop/secrets", secret)
            .unwrap()
            .build()
            .unwrap();
        let service_name = conn.unique_name().unwrap().clone();

        {
            let conn = blocking::Connection::session().unwrap();
            #[super::dbus_proxy(
                interface = "org.freedesktop.Secret.Service",
                assume_defaults = true,
                gen_async = false
            )]
            trait Secret {
                fn open_session(
                    &self,
                    algorithm: &str,
                    input: &zvariant::Value<'_>,
                ) -> zbus::Result<(OwnedValue, OwnedObjectPath)>;
            }

            let proxy = SecretProxy::builder(&conn)
                .destination(UniqueName::from(service_name))
                .unwrap()
                .path("/org/freedesktop/secrets")
                .unwrap()
                .build()
                .unwrap();

            trace!("Calling open_session");
            proxy.open_session("plain", &Value::from("")).unwrap();
            trace!("Called open_session");
        };
    }

    // This one we just want to see if it builds, no need to run it. For details see:
    //
    // https://github.com/dbus2/zbus/issues/121
    #[test]
    #[ignore]
    fn issue_121() {
        use crate::dbus_proxy;

        #[dbus_proxy(interface = "org.freedesktop.IBus", assume_defaults = true)]
        trait IBus {
            /// CurrentInputContext property
            #[dbus_proxy(property)]
            fn current_input_context(&self) -> zbus::Result<OwnedObjectPath>;

            /// Engines property
            #[dbus_proxy(property)]
            fn engines(&self) -> zbus::Result<Vec<zvariant::OwnedValue>>;
        }
    }

    #[test]
    #[timeout(15000)]
    fn issue_122() {
        let conn = blocking::Connection::session().unwrap();
        let stream = MessageIterator::from(&conn);

        #[allow(clippy::mutex_atomic)]
        let pair = Arc::new((Mutex::new(false), Condvar::new()));
        let pair2 = Arc::clone(&pair);

        let child = std::thread::spawn(move || {
            {
                let (lock, cvar) = &*pair2;
                let mut started = lock.lock().unwrap();
                *started = true;
                cvar.notify_one();
            }

            for m in stream {
                let msg = m.unwrap();
                let hdr = msg.header().unwrap();

                if hdr.member().unwrap().map(|m| m.as_str()) == Some("ZBusIssue122") {
                    break;
                }
            }
        });

        // Wait for the receiving thread to start up.
        let (lock, cvar) = &*pair;
        let mut started = lock.lock().unwrap();
        while !*started {
            started = cvar.wait(started).unwrap();
        }
        // Still give it some milliseconds to ensure it's already blocking on receive_message call
        // when we send a message.
        std::thread::sleep(std::time::Duration::from_millis(100));

        let destination = conn.unique_name().map(UniqueName::<'_>::from);
        let msg = Message::method(
            None::<()>,
            destination,
            "/does/not/matter",
            None::<()>,
            "ZBusIssue122",
            &(),
        )
        .unwrap();
        conn.send_message(msg).unwrap();

        child.join().unwrap();
    }

    #[test]
    #[ignore]
    fn issue_81() {
        use zbus::dbus_proxy;
        use zvariant::{OwnedValue, Type};

        #[derive(
            Debug, PartialEq, Eq, Clone, Type, OwnedValue, serde::Serialize, serde::Deserialize,
        )]
        pub struct DbusPath {
            id: String,
            path: OwnedObjectPath,
        }

        #[dbus_proxy(assume_defaults = true)]
        trait Session {
            #[dbus_proxy(property)]
            fn sessions_tuple(&self) -> zbus::Result<(String, String)>;

            #[dbus_proxy(property)]
            fn sessions_struct(&self) -> zbus::Result<DbusPath>;
        }
    }

    #[test]
    #[timeout(15000)]
    fn issue173() {
        // Tests the fix for https://github.com/dbus2/zbus/issues/173
        //
        // The issue is caused by proxy not keeping track of its destination's owner changes
        // (service restart) and failing to receive signals as a result.
        let (tx, rx) = channel();
        let child = std::thread::spawn(move || {
            let conn = blocking::Connection::session().unwrap();
            #[super::dbus_proxy(
                interface = "org.freedesktop.zbus.ComeAndGo",
                default_service = "org.freedesktop.zbus.ComeAndGo",
                default_path = "/org/freedesktop/zbus/ComeAndGo"
            )]
            trait ComeAndGo {
                #[dbus_proxy(signal)]
                fn the_signal(&self) -> zbus::Result<()>;
            }

            let proxy = ComeAndGoProxyBlocking::new(&conn).unwrap();
            let signals = proxy.receive_the_signal().unwrap();
            tx.send(()).unwrap();

            // We receive two signals, each time from different unique names. W/o the fix for
            // issue#173, the second iteration hangs.
            for _ in signals.take(2) {
                tx.send(()).unwrap();
            }
        });

        struct ComeAndGo;
        #[super::dbus_interface(name = "org.freedesktop.zbus.ComeAndGo")]
        impl ComeAndGo {
            #[dbus_interface(signal)]
            async fn the_signal(signal_ctxt: &SignalContext<'_>) -> zbus::Result<()>;
        }

        rx.recv().unwrap();
        for _ in 0..2 {
            let conn = blocking::ConnectionBuilder::session()
                .unwrap()
                .serve_at("/org/freedesktop/zbus/ComeAndGo", ComeAndGo)
                .unwrap()
                .name("org.freedesktop.zbus.ComeAndGo")
                .unwrap()
                .build()
                .unwrap();

            let iface_ref = conn
                .object_server()
                .interface::<_, ComeAndGo>("/org/freedesktop/zbus/ComeAndGo")
                .unwrap();
            block_on(ComeAndGo::the_signal(iface_ref.signal_context())).unwrap();

            rx.recv().unwrap();

            // Now we release the name ownership to use a different connection (i-e new unique
            // name).
            conn.release_name("org.freedesktop.zbus.ComeAndGo").unwrap();
        }

        child.join().unwrap();
    }

    #[test]
    #[timeout(15000)]
    fn uncached_property() {
        block_on(test_uncached_property()).unwrap();
    }

    async fn test_uncached_property() -> Result<()> {
        // A dummy boolean test service. It starts as `false` and can be
        // flipped to `true`. Two properties can access the inner value, with
        // and without caching.
        #[derive(Default)]
        struct ServiceUncachedPropertyTest(bool);
        #[crate::dbus_interface(name = "org.freedesktop.zbus.UncachedPropertyTest")]
        impl ServiceUncachedPropertyTest {
            #[dbus_interface(property)]
            fn cached_prop(&self) -> bool {
                self.0
            }
            #[dbus_interface(property)]
            fn uncached_prop(&self) -> bool {
                self.0
            }
            async fn set_inner_to_true(&mut self) -> zbus::fdo::Result<()> {
                self.0 = true;
                Ok(())
            }
        }

        #[crate::dbus_proxy(
            interface = "org.freedesktop.zbus.UncachedPropertyTest",
            default_service = "org.freedesktop.zbus.UncachedPropertyTest",
            default_path = "/org/freedesktop/zbus/UncachedPropertyTest"
        )]
        trait UncachedPropertyTest {
            #[dbus_proxy(property)]
            fn cached_prop(&self) -> zbus::Result<bool>;

            #[dbus_proxy(property(emits_changed_signal = "false"))]
            fn uncached_prop(&self) -> zbus::Result<bool>;

            fn set_inner_to_true(&self) -> zbus::Result<()>;
        }

        let service = crate::ConnectionBuilder::session()
            .unwrap()
            .serve_at(
                "/org/freedesktop/zbus/UncachedPropertyTest",
                ServiceUncachedPropertyTest(false),
            )
            .unwrap()
            .build()
            .await
            .unwrap();

        let dest = service.unique_name().unwrap();

        let client_conn = crate::Connection::session().await.unwrap();
        let client = UncachedPropertyTestProxy::builder(&client_conn)
            .destination(dest)
            .unwrap()
            .build()
            .await
            .unwrap();

        // Query properties; this populates the cache too.
        assert!(!client.cached_prop().await.unwrap());
        assert!(!client.uncached_prop().await.unwrap());

        // Flip the inner value so we can observe the different semantics of
        // the two properties.
        client.set_inner_to_true().await.unwrap();

        // Query properties again; the first one should incur a stale read from
        // cache, while the second one should be able to read the live/updated
        // value.
        assert!(!client.cached_prop().await.unwrap());
        assert!(client.uncached_prop().await.unwrap());

        Ok(())
    }

    #[test]
    #[timeout(15000)]
    fn issue_260() {
        // Low-level server example in the book doesn't work. The reason was that
        // `Connection::request_name` implicitly created the associated `ObjectServer` to avoid
        // #68. This meant that the `ObjectServer` ended up replying to the incoming method call
        // with an error, before the service code could do so.
        block_on(async {
            let connection = Connection::session().await?;

            connection.request_name("org.zbus.Issue260").await?;

            futures_util::try_join!(
                issue_260_service(&connection),
                issue_260_client(&connection),
            )?;

            Ok::<(), zbus::Error>(())
        })
        .unwrap();
    }

    async fn issue_260_service(connection: &Connection) -> Result<()> {
        use futures_util::stream::TryStreamExt;

        let mut stream = zbus::MessageStream::from(connection);
        while let Some(msg) = stream.try_next().await? {
            let msg_header = msg.header()?;

            match msg_header.message_type()? {
                zbus::MessageType::MethodCall => {
                    connection.reply(&msg, &()).await?;

                    break;
                }
                _ => continue,
            }
        }

        Ok(())
    }

    async fn issue_260_client(connection: &Connection) -> Result<()> {
        zbus::Proxy::new(
            connection,
            "org.zbus.Issue260",
            "/org/zbus/Issue260",
            "org.zbus.Issue260",
        )
        .await?
        .call("Whatever", &())
        .await?;
        Ok(())
    }

    #[test(tokio::test(flavor = "multi_thread", worker_threads = 2))]
    // Issue specific to tokio runtime.
    #[cfg(all(unix, feature = "tokio"))]
    #[instrument]
    async fn issue_279() {
        // On failure to read from the socket, we were closing the error channel from the sender
        // side and since the underlying tokio API doesn't provide a `close` method on the sender,
        // the async-channel abstraction was achieving this through calling `close` on receiver,
        // which is behind an async mutex and we end up with a deadlock.
        use crate::{ConnectionBuilder, MessageStream};
        use futures_util::{stream::TryStreamExt, try_join};
        use tokio::net::UnixStream;

        let guid = crate::Guid::generate();
        let (p0, p1) = UnixStream::pair().unwrap();

        let server = ConnectionBuilder::unix_stream(p0)
            .server(&guid)
            .p2p()
            .build();
        let client = ConnectionBuilder::unix_stream(p1).p2p().build();
        let (client, server) = try_join!(client, server).unwrap();
        let mut stream = MessageStream::from(client);
        let next_msg_fut = stream.try_next();

        drop(server);

        assert!(matches!(next_msg_fut.await, Err(_)));
    }

    #[test(tokio::test(flavor = "multi_thread"))]
    // Issue specific to tokio runtime.
    #[cfg(all(unix, feature = "tokio"))]
    #[instrument]
    async fn issue_310() {
        // The issue was we were deadlocking on fetching the new property value after invalidation.
        // This turned out to be caused by us trying to grab a read lock on resource while holding
        // a write lock. Thanks to connman for being weird and invalidating the property just before
        // updating it, so this issue could be exposed.
        use futures_util::StreamExt;
        use zbus::ConnectionBuilder;

        struct Station(u64);

        #[zbus::dbus_interface(name = "net.connman.iwd.Station")]
        impl Station {
            #[dbus_interface(property)]
            fn connected_network(&self) -> OwnedObjectPath {
                format!("/net/connman/iwd/0/33/Network/{}", self.0)
                    .try_into()
                    .unwrap()
            }
        }

        #[zbus::dbus_proxy(
            interface = "net.connman.iwd.Station",
            default_service = "net.connman.iwd"
        )]
        trait Station {
            #[dbus_proxy(property)]
            fn connected_network(&self) -> zbus::Result<OwnedObjectPath>;
        }
        let connection = ConnectionBuilder::session()
            .unwrap()
            .serve_at("/net/connman/iwd/0/33", Station(0))
            .unwrap()
            .name("net.connman.iwd")
            .unwrap()
            .build()
            .await
            .unwrap();
        let event = Arc::new(event_listener::Event::new());
        let conn_clone = connection.clone();
        let event_clone = event.clone();
        tokio::spawn(async move {
            for _ in 0..10 {
                let listener = event_clone.listen();
                let iface_ref = conn_clone
                    .object_server()
                    .interface::<_, Station>("/net/connman/iwd/0/33")
                    .await
                    .unwrap();

                {
                    let iface = iface_ref.get().await;
                    iface
                        .connected_network_invalidate(iface_ref.signal_context())
                        .await
                        .unwrap();
                    iface
                        .connected_network_changed(iface_ref.signal_context())
                        .await
                        .unwrap();
                }
                listener.await;
                iface_ref.get_mut().await.0 += 1;
            }
        });

        let station = StationProxy::builder(&connection)
            .path("/net/connman/iwd/0/33")
            .unwrap()
            .build()
            .await
            .unwrap();

        let mut changes = station.receive_connected_network_changed().await;

        let mut last_received = 0;
        while last_received < 9 {
            let change = changes.next().await.unwrap();
            let path = change.get().await.unwrap();
            let received: u64 = path
                .split('/')
                .last()
                .unwrap()
                .parse()
                .expect("invalid path");
            assert!(received >= last_received);
            last_received = received;
            event.notify(1);
        }
    }
}
