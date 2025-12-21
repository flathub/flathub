// Take a look at the license at the top of the repository in the LICENSE file.

#[cfg(unix)]
#[test]
fn test_gdbus_peer_connection() {
    use gio::{
        glib::{self, VariantTy},
        prelude::*,
        DBusConnection, DBusConnectionFlags, DBusNodeInfo, Socket,
    };
    use std::os::unix::net::UnixStream;

    const EXAMPLE_XML: &str = r#"
  <node>
    <interface name='com.github.gtk_rs'>
      <property type='i' name='Number' access='readwrite' />
      <method name='Hello'>
        <arg type='s' name='name' direction='in'/>
        <arg type='s' name='greet' direction='out'/>
      </method>
    </interface>
  </node>
"#;

    pub async fn spawn_server(fd: UnixStream) -> DBusConnection {
        let socket = Socket::from_fd(fd.into()).unwrap();
        let socket_connection = socket.connection_factory_create_connection();

        let guid = gio::dbus_generate_guid();

        dbg!("server connecting");

        let connection = DBusConnection::new_future(
            &socket_connection,
            Some(&guid),
            DBusConnectionFlags::AUTHENTICATION_SERVER
                .union(DBusConnectionFlags::DELAY_MESSAGE_PROCESSING),
            None,
        )
        .await
        .unwrap();

        dbg!("server connected");

        let interface_info = DBusNodeInfo::for_xml(EXAMPLE_XML)
            .unwrap()
            .lookup_interface("com.github.gtk_rs")
            .unwrap();

        let _id = connection
            .register_object("/com/github/gtk_rs", &interface_info)
            .method_call(
                |_connection,
                 _sender,
                 _object_path,
                 _interface_name,
                 _method_name,
                 parameters,
                 invocation| {
                    dbg!(
                        "method_call",
                        _sender,
                        _object_path,
                        _interface_name,
                        _method_name,
                        &parameters,
                        &invocation
                    );

                    let name = parameters.child_get::<String>(0);
                    invocation.return_value(Some(&(format!("Hello {name}!"),).to_variant()));
                },
            )
            .property({
                |_connection, _sender, _object_path, _interface_name, _property_name| {
                    dbg!(
                        "get_property",
                        _sender,
                        _object_path,
                        _interface_name,
                        _property_name
                    );
                    assert_eq!(_property_name, "Number");
                    123.to_variant()
                }
            })
            .set_property({
                |_connection, _sender, _object_path, _interface_name, _property_name, _value| {
                    dbg!(
                        "set_property",
                        _sender,
                        _object_path,
                        _interface_name,
                        _property_name,
                        &_value
                    );
                    assert_eq!(_property_name, "Number");
                    assert_eq!(_value, 456.to_variant());
                    true
                }
            })
            .build()
            .unwrap();

        dbg!("server starts message processing");

        connection.start_message_processing();

        dbg!("server awaiting calls");

        connection
    }

    pub async fn spawn_client(fd: UnixStream) -> DBusConnection {
        let socket_client = Socket::from_fd(fd.into()).unwrap();
        let socket_connection_client = socket_client.connection_factory_create_connection();

        dbg!("client connecting");

        let connection = DBusConnection::new_future(
            &socket_connection_client,
            None,
            DBusConnectionFlags::AUTHENTICATION_CLIENT,
            None,
        )
        .await
        .unwrap();

        dbg!("client connected");

        connection
    }

    let ctx = glib::MainContext::default();

    let (x, y) = std::os::unix::net::UnixStream::pair().unwrap();

    x.set_nonblocking(true).unwrap();
    y.set_nonblocking(true).unwrap();

    ctx.block_on(async move {
        let ctx = glib::MainContext::default();

        let server = ctx.spawn_local(spawn_server(x));
        let client = ctx.spawn_local(spawn_client(y));

        let server = server.await.unwrap();
        let client = client.await.unwrap();

        dbg!("calling method");

        let result = client
            .call_future(
                None,
                "/com/github/gtk_rs",
                "com.github.gtk_rs",
                "Hello",
                Some(&("World",).into()),
                Some(VariantTy::new("(s)").unwrap()),
                gio::DBusCallFlags::NONE,
                10000,
            )
            .await
            .unwrap();

        dbg!("method called");

        dbg!(&result);

        dbg!("getting property");

        let getresult = client
            .call_future(
                None,
                "/com/github/gtk_rs",
                "org.freedesktop.DBus.Properties",
                "Get",
                Some(&("com.github.gtk_rs", "Number").to_variant()),
                Some(VariantTy::new("(v)").unwrap()),
                gio::DBusCallFlags::NONE,
                10000,
            )
            .await
            .unwrap();

        assert_eq!(getresult, (123.to_variant(),).to_variant());

        dbg!("setting property");

        let setresult = client
            .call_future(
                None,
                "/com/github/gtk_rs",
                "org.freedesktop.DBus.Properties",
                "Set",
                Some(&("com.github.gtk_rs", "Number", 456.to_variant()).to_variant()),
                None,
                gio::DBusCallFlags::NONE,
                10000,
            )
            .await
            .unwrap();

        assert_eq!(setresult, ().to_variant());

        dbg!("closing client");
        client.close_future().await.unwrap();
        dbg!("closed client, closing server");
        server.close_future().await.unwrap();
        dbg!("closed server");

        drop(client);
        drop(server);

        assert_eq!(result.child_get::<String>(0), "Hello World!");

        glib::timeout_future_with_priority(
            glib::Priority::LOW,
            std::time::Duration::from_millis(50),
        )
        .await;
    });
}
