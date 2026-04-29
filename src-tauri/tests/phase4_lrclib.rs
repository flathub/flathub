use openkara_lib::lyrics::lrclib::{LrcLibClient, LyricsLookupQuery};

#[test]
fn fetches_synced_lyrics_from_lrclib_get_endpoint() {
    let mut server = mockito::Server::new();
    let mock = server
        .mock("GET", "/api/get")
        .match_query(mockito::Matcher::AllOf(vec![
            mockito::Matcher::UrlEncoded("track_name".into(), "Yellow".into()),
            mockito::Matcher::UrlEncoded("artist_name".into(), "Coldplay".into()),
            mockito::Matcher::UrlEncoded("album_name".into(), "Parachutes".into()),
            mockito::Matcher::UrlEncoded("duration".into(), "267".into()),
        ]))
        .match_header(
            "user-agent",
            format!("OpenKara/{}", env!("CARGO_PKG_VERSION")).as_str(),
        )
        .with_status(200)
        .with_header("content-type", "application/json")
        .with_body(
            r#"{
                "id": 16233,
                "trackName": "Yellow",
                "artistName": "Coldplay",
                "albumName": "Parachutes",
                "duration": 267.0,
                "instrumental": false,
                "plainLyrics": "Look at the stars",
                "syncedLyrics": "[00:35.66] Look at the stars"
            }"#,
        )
        .create();

    let client = LrcLibClient::new(server.url());
    let lyrics = client
        .fetch_by_track(&LyricsLookupQuery {
            track_name: "Yellow".to_owned(),
            artist_name: "Coldplay".to_owned(),
            album_name: Some("Parachutes".to_owned()),
            duration_seconds: Some(267),
        })
        .expect("LRCLIB fetch should succeed")
        .expect("lyrics should be returned");

    assert_eq!(lyrics.track_name, "Yellow");
    assert_eq!(lyrics.artist_name, "Coldplay");
    assert_eq!(
        lyrics.synced_lyrics.as_deref(),
        Some("[00:35.66] Look at the stars")
    );

    mock.assert();
}
