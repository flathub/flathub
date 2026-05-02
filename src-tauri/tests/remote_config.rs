use openkara_lib::config::{
    load_config, AppConfig, RegisteredLibrary, RemoteLibraryConnectionConfig,
    RemoteLibraryProvider,
};
use serde_json::Value;
use std::fs;

#[test]
fn remote_registered_library_serializes_as_discriminated_union() {
    let library = RegisteredLibrary::remote(
        "library-remote-1".to_owned(),
        "Drive".to_owned(),
        RemoteLibraryProvider::GoogleDrive,
        "acct-1".to_owned(),
        "folders/root".to_owned(),
        "OpenKara Drive".to_owned(),
        Some(RemoteLibraryConnectionConfig::GoogleDrive {
            oauth_client_id: "client-123.apps.googleusercontent.com".to_owned(),
        }),
        Some("/tmp/cache/openkara.db".to_owned()),
        Some("rev-7".to_owned()),
    );

    let json = serde_json::to_value(&library).expect("remote library should serialize");
    let object = json.as_object().expect("remote library should serialize as object");

    assert_eq!(object.get("kind"), Some(&Value::String("remote".to_owned())));
    assert_eq!(
        object.get("provider"),
        Some(&Value::String("google_drive".to_owned()))
    );
    assert_eq!(
        object.get("remote_root_locator"),
        Some(&Value::String("folders/root".to_owned()))
    );
    assert!(!object.contains_key("root_path"));

    let round_tripped: RegisteredLibrary =
        serde_json::from_value(json).expect("remote library should deserialize");
    assert_eq!(round_tripped, library);
}

#[test]
fn legacy_library_path_is_migrated_to_a_local_registry_entry() {
    let temp_dir = tempfile::tempdir().expect("temp dir should create");
    let config_path = temp_dir.path().join("config.json");

    fs::write(
        &config_path,
        r#"{
            "library_path": "/Users/test/Music/Legacy",
            "libraries": [],
            "active_library_id": null
        }"#,
    )
    .expect("legacy config should write");

    let config = load_config(temp_dir.path())
        .expect("legacy config should load")
        .expect("legacy config should exist");

    assert!(config.library_path.is_none());
    assert_eq!(config.libraries.len(), 1);
    assert!(matches!(config.libraries[0], RegisteredLibrary::Local { .. }));
    assert_eq!(config.active_library(), config.libraries.first());
}

#[test]
fn remote_library_entries_round_trip_through_app_config() {
    let config = AppConfig {
        libraries: vec![RegisteredLibrary::remote(
            "library-remote-1".to_owned(),
            "Dropbox".to_owned(),
            RemoteLibraryProvider::Dropbox,
            "acct-2".to_owned(),
            "apps/openkara".to_owned(),
            "OpenKara Dropbox".to_owned(),
            Some(RemoteLibraryConnectionConfig::Dropbox {
                app_key: "dropbox-app-key".to_owned(),
            }),
            None,
            Some("rev-9".to_owned()),
        )],
        active_library_id: Some("library-remote-1".to_owned()),
        ..AppConfig::default()
    };

    let json = serde_json::to_string(&config).expect("config should serialize");
    let loaded: AppConfig = serde_json::from_str(&json).expect("config should deserialize");

    assert_eq!(loaded.active_library_id.as_deref(), Some("library-remote-1"));
    assert_eq!(loaded.libraries.len(), 1);
    assert!(matches!(loaded.libraries[0], RegisteredLibrary::Remote { .. }));
}
