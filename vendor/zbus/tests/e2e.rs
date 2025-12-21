#![allow(clippy::disallowed_names)]
#[cfg(all(unix, not(feature = "tokio")))]
use std::os::unix::net::UnixStream;
use std::{collections::HashMap, convert::TryInto};
#[cfg(all(unix, feature = "tokio"))]
use tokio::net::UnixStream;

use event_listener::Event;
use futures_util::{StreamExt, TryStreamExt};
use ntest::timeout;
use serde::{Deserialize, Serialize};
use test_log::test;
use tokio::sync::mpsc::{channel, Sender};
use tracing::{debug, instrument};
use zbus::{
    block_on,
    fdo::{ObjectManager, ObjectManagerProxy},
    DBusError, MessageBuilder, MessageStream, ResponseDispatchNotifier,
};
use zvariant::{DeserializeDict, OwnedValue, SerializeDict, Str, Type, Value};

use zbus::{
    dbus_interface, dbus_proxy, CacheProperties, Connection, ConnectionBuilder, InterfaceRef,
    MessageHeader, MessageType, ObjectServer, SignalContext,
};

#[derive(Debug, Deserialize, Serialize, Type)]
pub struct ArgStructTest {
    foo: i32,
    bar: String,
}

// Mimic a NetworkManager interface property that's a dict. This tests ability to use a custom
// dict type using the `Type` And `*Dict` macros (issue #241).
#[derive(DeserializeDict, SerializeDict, Type, Debug, Value, OwnedValue, PartialEq, Eq)]
#[zvariant(signature = "dict")]
pub struct IP4Adress {
    prefix: u32,
    address: String,
}

// To test property setter for types with lifetimes.
#[derive(Serialize, Deserialize, Type, Debug, Value, OwnedValue, PartialEq, Eq)]
pub struct RefType<'a> {
    #[serde(borrow)]
    field1: Str<'a>,
}

#[dbus_proxy(assume_defaults = true, gen_blocking = true)]
trait MyIface {
    fn ping(&self) -> zbus::Result<u32>;

    fn quit(&self) -> zbus::Result<()>;

    fn test_header(&self) -> zbus::Result<()>;

    fn test_error(&self) -> zbus::Result<()>;

    fn test_single_struct_arg(&self, arg: ArgStructTest) -> zbus::Result<()>;

    fn test_single_struct_ret(&self) -> zbus::Result<ArgStructTest>;

    fn test_multi_ret(&self) -> zbus::Result<(i32, String)>;

    fn test_response_notify(&self) -> zbus::Result<String>;

    fn test_hashmap_return(&self) -> zbus::Result<HashMap<String, String>>;

    fn create_obj(&self, key: &str) -> zbus::Result<()>;

    fn destroy_obj(&self, key: &str) -> zbus::Result<()>;

    #[dbus_proxy(property)]
    fn count(&self) -> zbus::Result<u32>;

    #[dbus_proxy(property)]
    fn set_count(&self, count: u32) -> zbus::Result<()>;

    #[dbus_proxy(property)]
    fn hash_map(&self) -> zbus::Result<HashMap<String, String>>;

    #[dbus_proxy(property)]
    fn address_data(&self) -> zbus::Result<IP4Adress>;

    #[dbus_proxy(property)]
    fn set_address_data(&self, addr: IP4Adress) -> zbus::Result<()>;

    #[dbus_proxy(property)]
    fn address_data2(&self) -> zbus::Result<IP4Adress>;

    #[dbus_proxy(property)]
    fn str_prop(&self) -> zbus::Result<String>;

    #[dbus_proxy(property)]
    fn set_str_prop(&self, str_prop: &str) -> zbus::Result<()>;

    #[dbus_proxy(property)]
    fn ref_type(&self) -> zbus::Result<RefType<'_>>;

    #[dbus_proxy(property)]
    fn set_ref_type(&self, ref_type: RefType<'_>) -> zbus::Result<()>;

    #[dbus_proxy(property)]
    fn fail_property(&self) -> zbus::Result<u32>;

    #[dbus_proxy(no_reply)]
    fn test_no_reply(&self) -> zbus::Result<()>;

    #[dbus_proxy(no_autostart)]
    fn test_no_autostart(&self) -> zbus::Result<()>;

    #[dbus_proxy(allow_interactive_auth)]
    fn test_interactive_auth(&self) -> zbus::Result<()>;
}

#[derive(Debug, Clone)]
enum NextAction {
    Quit,
    CreateObj(String),
    DestroyObj(String),
}

#[derive(Debug)]
struct MyIfaceImpl {
    next_tx: Sender<NextAction>,
    count: u32,
}

impl MyIfaceImpl {
    fn new(next_tx: Sender<NextAction>) -> Self {
        Self { next_tx, count: 0 }
    }
}

/// Custom D-Bus error type.
#[derive(Debug, DBusError)]
#[dbus_error(prefix = "org.freedesktop.MyIface.Error")]
enum MyIfaceError {
    SomethingWentWrong(String),
    #[dbus_error(zbus_error)]
    ZBus(zbus::Error),
}

#[dbus_interface(interface = "org.freedesktop.MyIface")]
impl MyIfaceImpl {
    #[instrument]
    async fn ping(&mut self, #[zbus(signal_context)] ctxt: SignalContext<'_>) -> u32 {
        self.count += 1;
        if self.count % 3 == 0 {
            MyIfaceImpl::alert_count(&ctxt, self.count)
                .await
                .expect("Failed to emit signal");
            debug!("emitted `AlertCount` signal.");
        } else {
            debug!("Didn't emit `AlertCount` signal.");
        }
        self.count
    }

    #[instrument]
    async fn quit(&self) {
        debug!("Client asked to quit.");
        self.next_tx.send(NextAction::Quit).await.unwrap();
    }

    #[instrument]
    fn test_header(&self, #[zbus(header)] header: MessageHeader<'_>) {
        debug!("`TestHeader` called.");
        assert_eq!(header.message_type().unwrap(), MessageType::MethodCall);
        assert_eq!(header.member().unwrap().unwrap(), "TestHeader");
    }

    #[instrument]
    fn test_error(&self) -> zbus::fdo::Result<()> {
        debug!("`TestError` called.");
        Err(zbus::fdo::Error::Failed("error raised".to_string()))
    }

    #[instrument]
    fn test_custom_error(&self) -> Result<(), MyIfaceError> {
        debug!("`TestCustomError` called.");
        Err(MyIfaceError::SomethingWentWrong("oops".to_string()))
    }

    #[instrument]
    fn test_single_struct_arg(
        &self,
        arg: ArgStructTest,
        #[zbus(header)] header: MessageHeader<'_>,
    ) -> zbus::fdo::Result<()> {
        debug!("`TestSingleStructArg` called.");
        assert_eq!(header.signature()?.unwrap(), "(is)");
        assert_eq!(arg.foo, 1);
        assert_eq!(arg.bar, "TestString");

        Ok(())
    }

    #[instrument]
    fn test_single_struct_ret(&self) -> zbus::fdo::Result<ArgStructTest> {
        debug!("`TestSingleStructRet` called.");
        Ok(ArgStructTest {
            foo: 42,
            bar: String::from("Meaning of life"),
        })
    }

    #[instrument]
    #[dbus_interface(out_args("foo", "bar"))]
    fn test_multi_ret(&self) -> zbus::fdo::Result<(i32, String)> {
        debug!("`TestMultiRet` called.");
        Ok((42, String::from("Meaning of life")))
    }

    #[instrument]
    fn test_response_notify(
        &self,
        #[zbus(connection)] conn: &Connection,
        #[zbus(signal_context)] ctxt: SignalContext<'_>,
    ) -> zbus::fdo::Result<ResponseDispatchNotifier<String>> {
        debug!("`TestResponseNotify` called.");
        let (response, listener) = ResponseDispatchNotifier::new(String::from("Meaning of life"));
        let ctxt = ctxt.to_owned();
        conn.executor()
            .spawn(
                async move {
                    listener.await;

                    Self::test_response_notified(ctxt).await.unwrap();
                },
                "TestResponseNotify",
            )
            .detach();

        Ok(response)
    }

    #[dbus_interface(signal)]
    async fn test_response_notified(ctxt: SignalContext<'_>) -> zbus::Result<()>;

    #[instrument]
    async fn test_hashmap_return(&self) -> zbus::fdo::Result<HashMap<String, String>> {
        debug!("`TestHashmapReturn` called.");
        let mut map = HashMap::new();
        map.insert("hi".into(), "hello".into());
        map.insert("bye".into(), "now".into());

        Ok(map)
    }

    #[instrument]
    async fn create_obj(&self, key: String) {
        debug!("`CreateObj` called.");
        self.next_tx.send(NextAction::CreateObj(key)).await.unwrap();
    }

    #[instrument]
    async fn create_obj_inside(
        &self,
        #[zbus(object_server)] object_server: &ObjectServer,
        key: String,
    ) {
        debug!("`CreateObjInside` called.");
        object_server
            .at(
                format!("/zbus/test/{key}"),
                MyIfaceImpl::new(self.next_tx.clone()),
            )
            .await
            .unwrap();
    }

    #[instrument]
    async fn destroy_obj(&self, key: String) {
        debug!("`DestroyObj` called.");
        self.next_tx
            .send(NextAction::DestroyObj(key))
            .await
            .unwrap();
    }

    #[instrument]
    #[dbus_interface(property)]
    fn set_count(&mut self, val: u32) -> zbus::fdo::Result<()> {
        debug!("`Count` setter called.");
        if val == 42 {
            return Err(zbus::fdo::Error::InvalidArgs("Tsss tsss!".to_string()));
        }
        self.count = val;
        Ok(())
    }

    #[instrument]
    #[dbus_interface(property)]
    fn count(&self) -> u32 {
        debug!("`Count` getter called.");
        self.count
    }

    #[instrument]
    #[dbus_interface(property)]
    async fn hash_map(&self) -> HashMap<String, String> {
        debug!("`HashMap` getter called.");
        self.test_hashmap_return().await.unwrap()
    }

    #[instrument]
    #[dbus_interface(property)]
    async fn fail_property(&self) -> zbus::fdo::Result<u32> {
        Err(zbus::fdo::Error::UnknownProperty(
            "FailProperty".to_string(),
        ))
    }

    #[instrument]
    #[dbus_interface(property)]
    fn address_data(&self) -> IP4Adress {
        debug!("`AddressData` getter called.");
        IP4Adress {
            address: "127.0.0.1".to_string(),
            prefix: 1234,
        }
    }

    #[instrument]
    #[dbus_interface(property)]
    fn set_address_data(&self, addr: IP4Adress) {
        debug!("`AddressData` setter called with {:?}", addr);
    }

    // On the bus, this should return the same value as address_data above. We want to test if
    // this works both ways.
    #[instrument]
    #[dbus_interface(property)]
    fn address_data2(&self) -> HashMap<String, OwnedValue> {
        debug!("`AddressData2` getter called.");
        let mut map = HashMap::new();
        map.insert("address".into(), Value::from("127.0.0.1").into());
        map.insert("prefix".into(), 1234u32.into());

        map
    }

    #[instrument]
    #[dbus_interface(property)]
    fn str_prop(&self) -> String {
        "Hello".to_string()
    }

    #[instrument]
    #[dbus_interface(property)]
    fn set_str_prop(&self, str_prop: &str) {
        debug!("`SetStrRef` called with {:?}", str_prop);
    }

    #[instrument]
    #[dbus_interface(property)]
    fn ref_prop(&self) -> RefType<'_> {
        RefType {
            field1: "Hello".into(),
        }
    }

    #[instrument]
    #[dbus_interface(property)]
    fn set_ref_prop(&self, ref_type: RefType<'_>) {
        debug!("`SetRefType` called with {:?}", ref_type);
    }

    #[instrument]
    fn test_no_reply(&self, #[zbus(header)] header: MessageHeader<'_>) {
        debug!("`TestNoReply` called");
        assert_eq!(
            header.message_type().unwrap(),
            zbus::MessageType::MethodCall
        );
        assert!(header
            .primary()
            .flags()
            .contains(zbus::MessageFlags::NoReplyExpected));
    }

    #[instrument]
    fn test_no_autostart(&self, #[zbus(header)] header: MessageHeader<'_>) {
        debug!("`TestNoAutostart` called");
        assert_eq!(
            header.message_type().unwrap(),
            zbus::MessageType::MethodCall
        );
        assert!(header
            .primary()
            .flags()
            .contains(zbus::MessageFlags::NoAutoStart));
    }

    #[instrument]
    fn test_interactive_auth(&self, #[zbus(header)] header: MessageHeader<'_>) {
        debug!("`TestInteractiveAuth` called");
        assert_eq!(
            header.message_type().unwrap(),
            zbus::MessageType::MethodCall
        );
        assert!(header
            .primary()
            .flags()
            .contains(zbus::MessageFlags::AllowInteractiveAuth));
    }

    #[dbus_interface(signal)]
    async fn alert_count(ctxt: &SignalContext<'_>, val: u32) -> zbus::Result<()>;
}

fn check_hash_map(map: HashMap<String, String>) {
    assert_eq!(map["hi"], "hello");
    assert_eq!(map["bye"], "now");
}

fn check_ipv4_address(address: IP4Adress) {
    assert_eq!(
        address,
        IP4Adress {
            address: "127.0.0.1".to_string(),
            prefix: 1234,
        }
    );
}

#[instrument]
async fn my_iface_test(conn: Connection, event: Event) -> zbus::Result<u32> {
    debug!("client side starting..");
    // Use low-level API for `TestResponseNotify` because we need to ensure that the signal is
    // always received after the response.
    let mut stream = MessageStream::from(&conn);
    let method = MessageBuilder::method_call("/org/freedesktop/MyService", "TestResponseNotify")?
        .interface("org.freedesktop.MyIface")?
        .destination("org.freedesktop.MyService")?
        .build(&())?;
    let serial = conn.send_message(method).await?;
    let mut method_returned = false;
    let mut signal_received = false;
    while !method_returned && !signal_received {
        let msg = stream.try_next().await?.unwrap();

        let hdr = msg.header()?;
        if hdr.message_type()? == MessageType::MethodReturn && hdr.reply_serial()? == Some(serial) {
            assert!(!signal_received);
            method_returned = true;
        } else if hdr.message_type()? == MessageType::Signal
            && hdr.interface()?.unwrap() == "org.freedesktop.MyService"
            && hdr.member()?.unwrap() == "TestResponseNotified"
        {
            assert!(method_returned);
            signal_received = true;
        }
    }
    drop(stream);

    let proxy = MyIfaceProxy::builder(&conn)
        .destination("org.freedesktop.MyService")?
        .path("/org/freedesktop/MyService")?
        // the server isn't yet running
        .cache_properties(CacheProperties::No)
        .build()
        .await?;
    debug!("Created: {:?}", proxy);
    let props_proxy = zbus::fdo::PropertiesProxy::builder(&conn)
        .destination("org.freedesktop.MyService")?
        .path("/org/freedesktop/MyService")?
        .build()
        .await?;
    debug!("Created: {:?}", props_proxy);

    let mut props_changed_stream = props_proxy.receive_properties_changed().await?;
    debug!("Created: {:?}", props_changed_stream);
    event.notify(1);
    debug!("Notified service that client is ready");

    match props_changed_stream.next().await {
        Some(changed) => {
            assert_eq!(
                *changed.args()?.changed_properties().keys().next().unwrap(),
                "Count"
            );
        }
        None => panic!(""),
    };
    drop(props_changed_stream);

    proxy.ping().await?;
    assert_eq!(proxy.count().await?, 1);
    assert_eq!(proxy.cached_count()?, None);

    proxy.test_header().await?;
    proxy
        .test_single_struct_arg(ArgStructTest {
            foo: 1,
            bar: "TestString".into(),
        })
        .await?;
    check_hash_map(proxy.test_hashmap_return().await?);
    check_hash_map(proxy.hash_map().await?);
    proxy
        .set_address_data(IP4Adress {
            address: "localhost".to_string(),
            prefix: 1234,
        })
        .await?;
    proxy.set_str_prop("This is an str ref").await?;
    check_ipv4_address(proxy.address_data().await?);
    check_ipv4_address(proxy.address_data2().await?);

    proxy.test_no_reply().await?;
    proxy.test_no_autostart().await?;
    proxy.test_interactive_auth().await?;

    let err = proxy.fail_property().await;
    assert_eq!(
        err.unwrap_err(),
        zbus::Error::FDO(Box::new(zbus::fdo::Error::UnknownProperty(
            "FailProperty".into()
        )))
    );

    #[cfg(any(feature = "xml", feature = "quick-xml"))]
    {
        let xml = proxy.introspect().await?;
        debug!("Introspection: {}", xml);
        #[cfg(all(feature = "xml", not(feature = "quick-xml")))]
        let node = zbus::xml::Node::from_reader(xml.as_bytes())?;
        #[cfg(feature = "quick-xml")]
        let node = zbus::quick_xml::Node::from_reader(xml.as_bytes())?;
        let ifaces = node.interfaces();
        let iface = ifaces
            .iter()
            .find(|i| i.name() == "org.freedesktop.MyIface")
            .unwrap();
        let methods = iface.methods();
        for method in methods {
            if method.name() != "TestSingleStructRet" && method.name() != "TestMultiRet" {
                continue;
            }
            let args = method.args();
            #[cfg(all(feature = "xml", not(feature = "quick-xml")))]
            let mut out_args = args.iter().filter(|a| a.direction().unwrap() == "out");
            #[cfg(feature = "quick-xml")]
            let mut out_args = args
                .iter()
                .filter(|a| a.direction().unwrap() == zbus::quick_xml::ArgDirection::Out);

            if method.name() == "TestSingleStructRet" {
                assert_eq!(args.len(), 1);
                assert_eq!(out_args.next().unwrap().ty(), "(is)");
                assert!(out_args.next().is_none());
            } else {
                assert_eq!(args.len(), 2);
                let foo = out_args.find(|a| a.name() == Some("foo")).unwrap();
                assert_eq!(foo.ty(), "i");
                let bar = out_args.find(|a| a.name() == Some("bar")).unwrap();
                assert_eq!(bar.ty(), "s");
            }
        }
    }
    // build-time check to see if macro is doing the right thing.
    let _ = proxy.test_single_struct_ret().await?.foo;
    let _ = proxy.test_multi_ret().await?.1;

    let val = proxy.ping().await?;

    let obj_manager_proxy = ObjectManagerProxy::builder(&conn)
        .destination("org.freedesktop.MyService")?
        .path("/zbus/test")?
        .build()
        .await?;
    debug!("Created: {:?}", obj_manager_proxy);
    let mut ifaces_added_stream = obj_manager_proxy.receive_interfaces_added().await?;
    debug!("Created: {:?}", ifaces_added_stream);

    // Must process in parallel, so the stream listener does not block receiving
    // the method return message.
    let (ifaces_added, _) = futures_util::future::join(
        async {
            let ret = ifaces_added_stream.next().await.unwrap();
            drop(ifaces_added_stream);
            ret
        },
        async {
            proxy.create_obj("MyObj").await.unwrap();
        },
    )
    .await;

    assert_eq!(ifaces_added.args()?.object_path(), "/zbus/test/MyObj");
    let args = ifaces_added.args()?;
    let ifaces = args.interfaces_and_properties();
    let _ = ifaces.get("org.freedesktop.MyIface").unwrap();
    // TODO: Check if the properties are correct.

    // issue#207: interface panics on incorrect number of args.
    assert!(proxy.call_method("CreateObj", &()).await.is_err());

    let my_obj_proxy = MyIfaceProxy::builder(&conn)
        .destination("org.freedesktop.MyService")?
        .path("/zbus/test/MyObj")?
        .build()
        .await?;
    debug!("Created: {:?}", my_obj_proxy);
    my_obj_proxy.receive_count_changed().await;
    // Calling this after creating the stream was panicking if the property doesn't get cached
    // before the call (MR !460).
    my_obj_proxy.cached_count()?;
    assert_eq!(my_obj_proxy.count().await?, 0);
    assert_eq!(my_obj_proxy.cached_count()?, Some(0));
    assert_eq!(
        my_obj_proxy.cached_property_raw("Count").as_deref(),
        Some(&Value::from(0u32))
    );
    my_obj_proxy.ping().await?;

    let mut ifaces_removed_stream = obj_manager_proxy.receive_interfaces_removed().await?;
    debug!("Created: {:?}", ifaces_removed_stream);
    // Must process in parallel, so the stream listener does not block receiving
    // the method return message.
    let (ifaces_removed, _) = futures_util::future::join(
        async {
            let ret = ifaces_removed_stream.next().await.unwrap();
            drop(ifaces_removed_stream);
            ret
        },
        async {
            proxy.destroy_obj("MyObj").await.unwrap();
        },
    )
    .await;

    let args = ifaces_removed.args()?;
    assert_eq!(args.object_path(), "/zbus/test/MyObj");
    assert_eq!(args.interfaces(), &["org.freedesktop.MyIface"]);

    assert!(my_obj_proxy.introspect().await.is_err());
    assert!(my_obj_proxy.ping().await.is_err());

    // Make sure methods modifying the ObjectServer can be called without
    // deadlocks.
    proxy
        .call_method("CreateObjInside", &("CreatedInside"))
        .await?;
    let created_inside_proxy = MyIfaceProxy::builder(&conn)
        .destination("org.freedesktop.MyService")?
        .path("/zbus/test/CreatedInside")?
        .build()
        .await?;
    created_inside_proxy.ping().await?;
    proxy.destroy_obj("CreatedInside").await?;

    proxy.quit().await?;
    Ok(val)
}

#[test]
#[timeout(15000)]
fn iface_and_proxy() {
    block_on(iface_and_proxy_(false));
}

#[cfg(unix)]
#[test]
#[timeout(15000)]
fn iface_and_proxy_unix_p2p() {
    block_on(iface_and_proxy_(true));
}

#[instrument]
async fn iface_and_proxy_(p2p: bool) {
    let event = event_listener::Event::new();
    let guid = zbus::Guid::generate();

    let (service_conn_builder, client_conn_builder) = if p2p {
        #[cfg(unix)]
        {
            let (p0, p1) = UnixStream::pair().unwrap();

            (
                ConnectionBuilder::unix_stream(p0).server(&guid).p2p(),
                ConnectionBuilder::unix_stream(p1).p2p(),
            )
        }

        #[cfg(windows)]
        {
            #[cfg(not(feature = "tokio"))]
            {
                let listener = std::net::TcpListener::bind("127.0.0.1:0").unwrap();
                let addr = listener.local_addr().unwrap();
                let p1 = std::net::TcpStream::connect(addr).unwrap();
                let p0 = listener.incoming().next().unwrap().unwrap();

                (
                    ConnectionBuilder::tcp_stream(p0).server(&guid).p2p(),
                    ConnectionBuilder::tcp_stream(p1).p2p(),
                )
            }

            #[cfg(feature = "tokio")]
            {
                let listener = tokio::net::TcpListener::bind("127.0.0.1:0").await.unwrap();
                let addr = listener.local_addr().unwrap();
                let p1 = tokio::net::TcpStream::connect(addr).await.unwrap();
                let p0 = listener.accept().await.unwrap().0;

                (
                    ConnectionBuilder::tcp_stream(p0).server(&guid).p2p(),
                    ConnectionBuilder::tcp_stream(p1).p2p(),
                )
            }
        }
    } else {
        let service_conn_builder = ConnectionBuilder::session()
            .unwrap()
            .name("org.freedesktop.MyService")
            .unwrap()
            .name("org.freedesktop.MyService.foo")
            .unwrap()
            .name("org.freedesktop.MyService.bar")
            .unwrap();
        let client_conn_builder = ConnectionBuilder::session().unwrap();

        (service_conn_builder, client_conn_builder)
    };
    debug!(
        "Client connection builder created: {:?}",
        client_conn_builder
    );
    debug!(
        "Service connection builder created: {:?}",
        service_conn_builder
    );
    let (next_tx, mut next_rx) = channel(64);
    let iface = MyIfaceImpl::new(next_tx.clone());
    let service_conn_builder = service_conn_builder
        .serve_at("/org/freedesktop/MyService", iface)
        .unwrap()
        .serve_at("/zbus/test", ObjectManager)
        .unwrap();
    debug!("ObjectServer set-up.");

    let (service_conn, client_conn) =
        futures_util::try_join!(service_conn_builder.build(), client_conn_builder.build(),)
            .unwrap();
    debug!("Client connection created: {:?}", client_conn);
    debug!("Service connection created: {:?}", service_conn);

    let listen = event.listen();
    let child = client_conn
        .executor()
        .spawn(my_iface_test(client_conn.clone(), event), "client_task");
    debug!("Child task spawned.");
    // Wait for the listener to be ready
    listen.await;
    debug!("Child task signaled it's ready.");

    let iface: InterfaceRef<MyIfaceImpl> = service_conn
        .object_server()
        .interface("/org/freedesktop/MyService")
        .await
        .unwrap();
    iface
        .get()
        .await
        .count_changed(iface.signal_context())
        .await
        .unwrap();
    debug!("`PropertiesChanged` emitted for `Count` property.");

    loop {
        MyIfaceImpl::alert_count(iface.signal_context(), 51)
            .await
            .unwrap();
        debug!("`AlertCount` signal emitted.");

        match next_rx.recv().await.unwrap() {
            NextAction::Quit => break,
            NextAction::CreateObj(key) => {
                let path = format!("/zbus/test/{key}");
                service_conn
                    .object_server()
                    .at(path.clone(), MyIfaceImpl::new(next_tx.clone()))
                    .await
                    .unwrap();
                debug!("Object `{path}` added.");
            }
            NextAction::DestroyObj(key) => {
                let path = format!("/zbus/test/{key}");
                service_conn
                    .object_server()
                    .remove::<MyIfaceImpl, _>(path.clone())
                    .await
                    .unwrap();
                debug!("Object `{path}` removed.");
            }
        }
    }
    debug!("Server done.");

    // don't close the connection before we end the loop
    drop(client_conn);
    debug!("Connection closed.");

    let val = child.await.unwrap();
    debug!("Client task done.");
    assert_eq!(val, 2);

    if p2p {
        debug!("p2p connection, no need to release names..");
        return;
    }

    // Release primary name explicitly and let others be released implicitly.
    assert_eq!(
        service_conn.release_name("org.freedesktop.MyService").await,
        Ok(true)
    );
    debug!("Bus name `org.freedesktop.MyService` released.");
    assert_eq!(
        service_conn
            .release_name("org.freedesktop.MyService.foo")
            .await,
        Ok(true)
    );
    debug!("Bus name `org.freedesktop.MyService.foo` released.");
    assert_eq!(
        service_conn
            .release_name("org.freedesktop.MyService.bar")
            .await,
        Ok(true)
    );
    debug!("Bus name `org.freedesktop.MyService.bar` released.");

    // Let's ensure all names were released.
    let proxy = zbus::fdo::DBusProxy::new(&service_conn).await.unwrap();
    debug!("DBusProxy created to ensure all names were released.");
    assert_eq!(
        proxy
            .name_has_owner("org.freedesktop.MyService".try_into().unwrap())
            .await,
        Ok(false)
    );
    assert_eq!(
        proxy
            .name_has_owner("org.freedesktop.MyService.foo".try_into().unwrap())
            .await,
        Ok(false)
    );
    assert_eq!(
        proxy
            .name_has_owner("org.freedesktop.MyService.bar".try_into().unwrap())
            .await,
        Ok(false)
    );
    debug!("Bus confirmed that all names were definitely released.");
}
