extern crate gettextrs;
#[macro_use]
extern crate lazy_static;

use gettextrs::{getters::*, *};

lazy_static! {
    // These tests work with global resource, that is set up here once,
    // and shouldn't be modified in tests.
    static ref SETUP: () = {
        setlocale(LocaleCategory::LcAll, "en_US.UTF-8");

        bindtextdomain("bound_domain", "/usr/local/share/locale").unwrap();

        bindtextdomain("initialized_domain", "/usr/local/share/locale").unwrap();
        textdomain("initialized_domain").unwrap();

        bind_textdomain_codeset("c_domain", "C").unwrap();
        bind_textdomain_codeset("utf-8_domain", "UTF-8").unwrap();
    };
}

#[test]
fn current_textdomain_works() {
    let _ = *SETUP;

    assert_eq!(
        current_textdomain().unwrap(),
        "initialized_domain".as_bytes()
    );
}

#[test]
fn domain_directory_works() {
    let _ = *SETUP;

    use std::path::PathBuf;

    assert_eq!(
        domain_directory("bound_domain").unwrap(),
        PathBuf::from("/usr/local/share/locale")
    );
}

#[test]
fn test_textdomain_codeset() {
    let _ = *SETUP;

    assert_eq!(
        textdomain_codeset("c_domain").unwrap(),
        Some("C".to_string())
    );

    assert_eq!(
        textdomain_codeset("utf-8_domain").unwrap(),
        Some("UTF-8".to_string())
    );
}

#[test]
fn gettext_fn() {
    let _ = *SETUP;

    assert_eq!(gettext("Hello, World!"), "Hello, World!");
}

#[test]
fn dgettext_fn() {
    let _ = *SETUP;

    assert_eq!(
        current_textdomain().unwrap(),
        "initialized_domain".as_bytes()
    );

    assert_eq!(dgettext("bound_domain", "Hello, World!"), "Hello, World!");
}

#[test]
fn dcgettext_fn() {
    let _ = *SETUP;

    assert_eq!(
        current_textdomain().unwrap(),
        "initialized_domain".as_bytes()
    );

    assert_eq!(
        dcgettext("bound_domain", "Hello, World!", LocaleCategory::LcMessages),
        "Hello, World!"
    );
}

#[test]
fn pgettext_fn() {
    let _ = *SETUP;

    assert_eq!(
        current_textdomain().unwrap(),
        "initialized_domain".as_bytes()
    );

    assert_eq!(pgettext("context", "Hello, World!"), "Hello, World!");
}

#[test]
fn ngettext_fn() {
    let _ = *SETUP;

    assert_eq!(
        current_textdomain().unwrap(),
        "initialized_domain".as_bytes()
    );

    assert_eq!(
        ngettext("Hello, World!", "Hello, Worlds!", 1),
        "Hello, World!"
    );
    assert_eq!(
        ngettext("Hello, World!", "Hello, Worlds!", 2),
        "Hello, Worlds!"
    );
}

#[test]
fn dngettext_fn() {
    let _ = *SETUP;

    assert_eq!(
        current_textdomain().unwrap(),
        "initialized_domain".as_bytes()
    );

    assert_eq!(
        dngettext("bound_domain", "Hello, World!", "Hello, Worlds!", 1),
        "Hello, World!"
    );
    assert_eq!(
        dngettext("bound_domain", "Hello, World!", "Hello, Worlds!", 2),
        "Hello, Worlds!"
    );
}

#[test]
fn dcngettext_fn() {
    let _ = *SETUP;

    assert_eq!(
        current_textdomain().unwrap(),
        "initialized_domain".as_bytes()
    );

    assert_eq!(
        dcngettext(
            "bound_domain",
            "Hello, World!",
            "Hello, Worlds!",
            1,
            LocaleCategory::LcMessages
        ),
        "Hello, World!"
    );
    assert_eq!(
        dcngettext(
            "bound_domain",
            "Hello, World!",
            "Hello, Worlds!",
            2,
            LocaleCategory::LcMessages
        ),
        "Hello, Worlds!"
    );
}

#[test]
fn npgettext_fn() {
    let _ = *SETUP;

    assert_eq!(
        current_textdomain().unwrap(),
        "initialized_domain".as_bytes()
    );

    assert_eq!(
        npgettext("context", "Hello, World!", "Hello, Worlds!", 1),
        "Hello, World!"
    );
    assert_eq!(
        npgettext("context", "Hello, World!", "Hello, Worlds!", 2),
        "Hello, Worlds!"
    );
}
