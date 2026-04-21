use openkara_lib::config::{
    AppConfig, RegisteredLibrary, RemoteLibraryConnectionConfig, RemoteLibraryProvider,
};

#[test]
fn serializes_local_and_remote_libraries_with_distinct_shapes() {
    let local = RegisteredLibrary::local(
        "/Users/test/Music/OpenKara".to_owned(),
        "OpenKara".to_owned(),
    );
    let remote = RegisteredLibrary::remote(
        "remote-library-id".to_owned(),
        "OpenKara Cloud".to_owned(),
        RemoteLibraryProvider::GoogleDrive,
        "account-1".to_owned(),
        "root-folder-id".to_owned(),
        "My Drive/OpenKara".to_owned(),
        Some(RemoteLibraryConnectionConfig::GoogleDrive {
            oauth_client_id: "client-123.apps.googleusercontent.com".to_owned(),
        }),
        Some("cached/openkara.db".to_owned()),
        Some("rev-1".to_owned()),
        Some("local-library-id".to_owned()),
    );

    let config = AppConfig {
        libraries: vec![local.clone(), remote.clone()],
        active_library_id: Some(remote.id().to_owned()),
        ..AppConfig::default()
    };

    let json = serde_json::to_string(&config).expect("config should serialize");
    let decoded: AppConfig = serde_json::from_str(&json).expect("config should deserialize");

    assert_eq!(decoded.libraries.len(), 2);
    assert!(matches!(decoded.libraries[0], RegisteredLibrary::Local { .. }));
    assert!(matches!(decoded.libraries[1], RegisteredLibrary::Remote { .. }));
    assert_eq!(decoded.active_library_id.as_deref(), Some(remote.id()));
    assert_eq!(decoded.active_library().map(|library| library.id()), Some(remote.id()));
}
