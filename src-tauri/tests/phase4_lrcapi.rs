use openkara_lib::lyrics::{
    lrcapi::LrcApiClient,
    lrclib::LyricsLookupQuery,
};

#[test]
fn fetches_highest_scoring_candidate_from_jsonapi() {
    let mut server = mockito::Server::new();
    let mock = server
        .mock("GET", "/jsonapi")
        .match_query(mockito::Matcher::AllOf(vec![
            mockito::Matcher::UrlEncoded("title".into(), "Yellow".into()),
            mockito::Matcher::UrlEncoded("artist".into(), "Coldplay".into()),
            mockito::Matcher::UrlEncoded("album".into(), "Parachutes".into()),
        ]))
        .with_status(200)
        .with_header("content-type", "application/json")
        .with_body(
            r#"[
                {
                    "id": "1",
                    "title": "Yellow",
                    "artist": "Coldplay",
                    "album": "Parachutes",
                    "score": 10.0,
                    "lrc": "[00:01.00] low score",
                    "lrc_ttml": "<tt>ignored</tt>",
                    "lyric_path": "/lyrics/low"
                },
                {
                    "id": "2",
                    "title": "Yellow",
                    "artist": "Coldplay",
                    "album": "Parachutes",
                    "score": 84.791534,
                    "lrc": "[00:33.642] Look at the stars",
                    "lrc_ttml": "<tt>ignored</tt>",
                    "lyric_path": "/lyrics/high"
                }
            ]"#,
        )
        .create();

    let client = LrcApiClient::new(server.url());
    let lyrics = client
        .fetch_by_track(&LyricsLookupQuery {
            track_name: "Yellow".to_owned(),
            artist_name: "Coldplay".to_owned(),
            album_name: Some("Parachutes".to_owned()),
            duration_seconds: None,
        })
        .expect("LrcApi fetch should succeed")
        .expect("lyrics should be returned");

    assert_eq!(lyrics.id, "2");
    assert_eq!(lyrics.score, 84.791534);
    assert!(lyrics.lrc.contains("[00:33.642] Look at the stars"));
    assert!(lyrics.lrc_ttml.is_some());
    assert_eq!(lyrics.lyric_path.as_deref(), Some("/lyrics/high"));

    mock.assert();
}

#[test]
fn treats_not_found_message_as_none() {
    let mut server = mockito::Server::new();
    let mock = server
        .mock("GET", "/jsonapi")
        .match_query(mockito::Matcher::Any)
        .with_status(200)
        .with_header("content-type", "application/json")
        .with_body(r#"{"message":"未找到歌词"}"#)
        .create();

    let client = LrcApiClient::new(server.url());
    let lyrics = client
        .fetch_by_track(&LyricsLookupQuery {
            track_name: "Missing".to_owned(),
            artist_name: "Nobody".to_owned(),
            album_name: None,
            duration_seconds: None,
        })
        .expect("LrcApi fetch should succeed");

    assert!(lyrics.is_none());

    mock.assert();
}
