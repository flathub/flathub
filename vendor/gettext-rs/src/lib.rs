//! # Safe Rust bindings for gettext.
//!
//! Usage:
//!
//! ```rust,no_run
//! use gettextrs::*;
//!
//! fn main() -> Result<(), Box<dyn std::error::Error>> {
//!     // Specify the name of the .mo file to use.
//!     textdomain("hellorust")?;
//!     // Ask gettext for UTF-8 strings. THIS CRATE CAN'T HANDLE NON-UTF-8 DATA!
//!     bind_textdomain_codeset("hellorust", "UTF-8")?;
//!
//!     // You could also use `TextDomain` builder which calls `textdomain` and
//!     // other functions for you:
//!     //
//!     // TextDomain::new("hellorust").init()?;
//!
//!     // `gettext()` simultaneously marks a string for translation and translates
//!     // it at runtime.
//!     println!("Translated: {}", gettext("Hello, world!"));
//!
//!     // gettext supports plurals, i.e. you can have different messages depending
//!     // on the number of items the message mentions. This even works for
//!     // languages that have more than one plural form, like Russian or Czech.
//!     println!("Singular: {}", ngettext("One thing", "Multiple things", 1));
//!     println!("Plural: {}", ngettext("One thing", "Multiple things", 2));
//!
//!     // gettext de-duplicates strings, i.e. the same string used multiple times
//!     // will have a single entry in the PO and MO files. However, the same words
//!     // might have different meaning depending on the context. To distinguish
//!     // between different contexts, gettext accepts an additional string:
//!     println!("With context: {}", pgettext("This is the context", "Hello, world!"));
//!     println!(
//!         "Plural with context: {}",
//!         npgettext("This is the context", "One thing", "Multiple things", 2));
//!
//!     Ok(())
//! }
//! ```
//!
//! ## UTF-8 is required
//!
//! By default, gettext converts results to the locale's codeset. Rust, on the other hand, always
//! encodes strings to UTF-8. The best way to bridge this gap is to ask gettext to convert strings
//! to UTF-8:
//!
//! ```rust,no_run
//! # use gettextrs::*;
//! # fn main() -> Result<(), Box<dyn std::error::Error>> {
//! bind_textdomain_codeset("hellorust", "UTF-8")?;
//! # Ok(())
//! # }
//! ```
//!
//! ...or using [`TextDomain`] builder:
//!
//! ```rust,no_run
//! # use gettextrs::*;
//! # fn main() -> Result<(), Box<dyn std::error::Error>> {
//! TextDomain::new("hellorust")
//!     .codeset("UTF-8") // Optional, the builder does this by default
//!     .init()?;
//! # Ok(())
//! # }
//! ```
//!
//! This crate doesn't do this for you because the encoding is a global setting; changing it can
//! affect other gettext calls in your program, like calls in C or C++ parts of your binary.
//!
//! If you don't do this, calls to `gettext()` and other functions might panic when they encounter
//! something that isn't UTF-8. They can also garble data as they interpret the other encoding as
//! UTF-8.
//!
//! Another thing you could do is change the locale, e.g. `setlocale(LocaleCategory::LcAll,
//! "fr_FR.UTF-8")`, but that would also hard-code the language, defeating the purpose of gettext:
//! if you know the language in advance, you could just write all your strings in that language and
//! be done with that.

extern crate locale_config;

extern crate gettext_sys as ffi;

use std::ffi::CStr;
use std::ffi::CString;
use std::io;
use std::os::raw::c_ulong;
use std::path::PathBuf;

mod text_domain;
pub use text_domain::{TextDomain, TextDomainError};
pub mod getters;

/// Locale category enum ported from locale.h.
#[derive(Debug, PartialEq, Clone, Copy)]
pub enum LocaleCategory {
    /// Character classification and case conversion.
    LcCType = 0,
    /// Non-monetary numeric formats.
    LcNumeric = 1,
    /// Date and time formats.
    LcTime = 2,
    /// Collation order.
    LcCollate = 3,
    /// Monetary formats.
    LcMonetary = 4,
    /// Formats of informative and diagnostic messages and interactive responses.
    LcMessages = 5,
    /// For all.
    LcAll = 6,
    /// Paper size.
    LcPaper = 7,
    /// Name formats.
    LcName = 8,
    /// Address formats and location information.
    LcAddress = 9,
    /// Telephone number formats.
    LcTelephone = 10,
    /// Measurement units (Metric or Other).
    LcMeasurement = 11,
    /// Metadata about the locale information.
    LcIdentification = 12,
}

/// Translate msgid to localized message from the default domain.
///
/// For more information, see [gettext(3)][].
///
/// [gettext(3)]: https://www.man7.org/linux/man-pages/man3/gettext.3.html
///
/// # Panics
///
/// Panics if:
///
/// * `msgid` contains an internal 0 byte, as such values can't be passed to the underlying C API;
/// * the result is not in UTF-8 (see [this note](./index.html#utf-8-is-required)).
pub fn gettext<T: Into<String>>(msgid: T) -> String {
    let msgid = CString::new(msgid.into()).expect("`msgid` contains an internal 0 byte");
    unsafe {
        CStr::from_ptr(ffi::gettext(msgid.as_ptr()))
            .to_str()
            .expect("gettext() returned invalid UTF-8")
            .to_owned()
    }
}

/// Translate msgid to localized message from the specified domain.
///
/// For more information, see [dgettext(3)][].
///
/// [dgettext(3)]: https://www.man7.org/linux/man-pages/man3/dgettext.3.html
///
/// # Panics
///
/// Panics if:
///
/// * `domainname` or `msgid` contain an internal 0 byte, as such values can't be passed to the
///     underlying C API;
/// * the result is not in UTF-8 (see [this note](./index.html#utf-8-is-required)).
pub fn dgettext<T, U>(domainname: T, msgid: U) -> String
where
    T: Into<String>,
    U: Into<String>,
{
    let domainname =
        CString::new(domainname.into()).expect("`domainname` contains an internal 0 byte");
    let msgid = CString::new(msgid.into()).expect("`msgid` contains an internal 0 byte");
    unsafe {
        CStr::from_ptr(ffi::dgettext(domainname.as_ptr(), msgid.as_ptr()))
            .to_str()
            .expect("dgettext() returned invalid UTF-8")
            .to_owned()
    }
}

/// Translate msgid to localized message from the specified domain using custom locale category.
///
/// For more information, see [dcgettext(3)][].
///
/// [dcgettext(3)]: https://www.man7.org/linux/man-pages/man3/dcgettext.3.html
///
/// # Panics
///
/// Panics if:
/// * `domainname` or `msgid` contain an internal 0 byte, as such values can't be passed to the
///     underlying C API;
/// * the result is not in UTF-8 (see [this note](./index.html#utf-8-is-required)).
pub fn dcgettext<T, U>(domainname: T, msgid: U, category: LocaleCategory) -> String
where
    T: Into<String>,
    U: Into<String>,
{
    let domainname =
        CString::new(domainname.into()).expect("`domainname` contains an internal 0 byte");
    let msgid = CString::new(msgid.into()).expect("`msgid` contains an internal 0 byte");
    unsafe {
        CStr::from_ptr(ffi::dcgettext(
            domainname.as_ptr(),
            msgid.as_ptr(),
            category as i32,
        ))
        .to_str()
        .expect("dcgettext() returned invalid UTF-8")
        .to_owned()
    }
}

/// Translate msgid to localized message from the default domain (with plural support).
///
/// For more information, see [ngettext(3)][].
///
/// [ngettext(3)]: https://www.man7.org/linux/man-pages/man3/ngettext.3.html
///
/// # Panics
///
/// Panics if:
/// * `msgid` or `msgid_plural` contain an internal 0 byte, as such values can't be passed to the
///     underlying C API;
/// * the result is not in UTF-8 (see [this note](./index.html#utf-8-is-required)).
pub fn ngettext<T, S>(msgid: T, msgid_plural: S, n: u32) -> String
where
    T: Into<String>,
    S: Into<String>,
{
    let msgid = CString::new(msgid.into()).expect("`msgid` contains an internal 0 byte");
    let msgid_plural =
        CString::new(msgid_plural.into()).expect("`msgid_plural` contains an internal 0 byte");
    unsafe {
        CStr::from_ptr(ffi::ngettext(
            msgid.as_ptr(),
            msgid_plural.as_ptr(),
            n as c_ulong,
        ))
        .to_str()
        .expect("ngettext() returned invalid UTF-8")
        .to_owned()
    }
}

/// Translate msgid to localized message from the specified domain (with plural support).
///
/// For more information, see [dngettext(3)][].
///
/// [dngettext(3)]: https://www.man7.org/linux/man-pages/man3/dngettext.3.html
///
/// # Panics
///
/// Panics if:
/// * `domainname`, `msgid`, or `msgid_plural` contain an internal 0 byte, as such values can't be
///     passed to the underlying C API;
/// * the result is not in UTF-8 (see [this note](./index.html#utf-8-is-required)).
pub fn dngettext<T, U, V>(domainname: T, msgid: U, msgid_plural: V, n: u32) -> String
where
    T: Into<String>,
    U: Into<String>,
    V: Into<String>,
{
    let domainname =
        CString::new(domainname.into()).expect("`domainname` contains an internal 0 byte");
    let msgid = CString::new(msgid.into()).expect("`msgid` contains an internal 0 byte");
    let msgid_plural =
        CString::new(msgid_plural.into()).expect("`msgid_plural` contains an internal 0 byte");
    unsafe {
        CStr::from_ptr(ffi::dngettext(
            domainname.as_ptr(),
            msgid.as_ptr(),
            msgid_plural.as_ptr(),
            n as c_ulong,
        ))
        .to_str()
        .expect("dngettext() returned invalid UTF-8")
        .to_owned()
    }
}

/// Translate msgid to localized message from the specified domain using custom locale category
/// (with plural support).
///
/// For more information, see [dcngettext(3)][].
///
/// [dcngettext(3)]: https://www.man7.org/linux/man-pages/man3/dcngettext.3.html
///
/// # Panics
///
/// Panics if:
/// * `domainname`, `msgid`, or `msgid_plural` contain an internal 0 byte, as such values can't be
///     passed to the underlying C API;
/// * the result is not in UTF-8 (see [this note](./index.html#utf-8-is-required)).
pub fn dcngettext<T, U, V>(
    domainname: T,
    msgid: U,
    msgid_plural: V,
    n: u32,
    category: LocaleCategory,
) -> String
where
    T: Into<String>,
    U: Into<String>,
    V: Into<String>,
{
    let domainname =
        CString::new(domainname.into()).expect("`domainname` contains an internal 0 byte");
    let msgid = CString::new(msgid.into()).expect("`msgid` contains an internal 0 byte");
    let msgid_plural =
        CString::new(msgid_plural.into()).expect("`msgid_plural` contains an internal 0 byte");
    unsafe {
        CStr::from_ptr(ffi::dcngettext(
            domainname.as_ptr(),
            msgid.as_ptr(),
            msgid_plural.as_ptr(),
            n as c_ulong,
            category as i32,
        ))
        .to_str()
        .expect("dcngettext() returned invalid UTF-8")
        .to_owned()
    }
}

/// Switch to the specific text domain.
///
/// Returns the current domain, after possibly changing it. (There's no trailing 0 byte in the
/// return value.)
///
/// If you want to *get* current domain, rather than set it, use [`getters::current_textdomain`].
///
/// For more information, see [textdomain(3)][].
///
/// [textdomain(3)]: https://www.man7.org/linux/man-pages/man3/textdomain.3.html
///
/// # Panics
///
/// Panics if `domainname` contains an internal 0 byte, as such values can't be passed to the
/// underlying C API.
pub fn textdomain<T: Into<Vec<u8>>>(domainname: T) -> Result<Vec<u8>, io::Error> {
    let domainname = CString::new(domainname).expect("`domainname` contains an internal 0 byte");
    unsafe {
        let result = ffi::textdomain(domainname.as_ptr());
        if result.is_null() {
            Err(io::Error::last_os_error())
        } else {
            Ok(CStr::from_ptr(result).to_bytes().to_owned())
        }
    }
}

/// Specify the directory that contains MO files for the given domain.
///
/// Returns the current directory for given domain, after possibly changing it.
///
/// If you want to *get* domain directory, rather than set it, use [`getters::domain_directory`].
///
/// For more information, see [bindtextdomain(3)][].
///
/// [bindtextdomain(3)]: https://www.man7.org/linux/man-pages/man3/bindtextdomain.3.html
///
/// # Panics
///
/// Panics if `domainname` or `dirname` contain an internal 0 byte, as such values can't be passed
/// to the underlying C API.
pub fn bindtextdomain<T, U>(domainname: T, dirname: U) -> Result<PathBuf, io::Error>
where
    T: Into<Vec<u8>>,
    U: Into<PathBuf>,
{
    let domainname = CString::new(domainname).expect("`domainname` contains an internal 0 byte");
    let dirname = dirname.into().into_os_string();

    #[cfg(windows)]
    {
        use std::ffi::OsString;
        use std::os::windows::ffi::{OsStrExt, OsStringExt};

        let mut dirname: Vec<u16> = dirname.encode_wide().collect();
        if dirname.contains(&0) {
            panic!("`dirname` contains an internal 0 byte");
        }
        // Trailing zero to mark the end of the C string.
        dirname.push(0);
        unsafe {
            let mut ptr = ffi::wbindtextdomain(domainname.as_ptr(), dirname.as_ptr());
            if ptr.is_null() {
                Err(io::Error::last_os_error())
            } else {
                let mut result = vec![];
                while *ptr != 0_u16 {
                    result.push(*ptr);
                    ptr = ptr.offset(1);
                }
                Ok(PathBuf::from(OsString::from_wide(&result)))
            }
        }
    }

    #[cfg(not(windows))]
    {
        use std::ffi::OsString;
        use std::os::unix::ffi::OsStringExt;

        let dirname = dirname.into_vec();
        let dirname = CString::new(dirname).expect("`dirname` contains an internal 0 byte");
        unsafe {
            let result = ffi::bindtextdomain(domainname.as_ptr(), dirname.as_ptr());
            if result.is_null() {
                Err(io::Error::last_os_error())
            } else {
                let result = CStr::from_ptr(result);
                Ok(PathBuf::from(OsString::from_vec(
                    result.to_bytes().to_vec(),
                )))
            }
        }
    }
}

/// Set current locale.
///
/// Returns an opaque string that describes the locale set. You can pass that string into
/// `setlocale()` later to set the same local again. `None` means the call failed (the underlying
/// API doesn't provide any details).
///
/// For more information, see [setlocale(3)][].
///
/// [setlocale(3)]: https://www.man7.org/linux/man-pages/man3/setlocale.3.html
///
/// # Panics
///
/// Panics if `locale` contains an internal 0 byte, as such values can't be passed to the
/// underlying C API.
pub fn setlocale<T: Into<Vec<u8>>>(category: LocaleCategory, locale: T) -> Option<Vec<u8>> {
    let c = CString::new(locale).expect("`locale` contains an internal 0 byte");
    unsafe {
        let ret = ffi::setlocale(category as i32, c.as_ptr());
        if ret.is_null() {
            None
        } else {
            Some(CStr::from_ptr(ret).to_bytes().to_owned())
        }
    }
}

/// Set encoding of translated messages.
///
/// Returns the current charset for given domain, after possibly changing it. `None` means no
/// codeset has been set.
///
/// If you want to *get* current encoding, rather than set it, use [`getters::textdomain_codeset`].
///
/// For more information, see [bind_textdomain_codeset(3)][].
///
/// [bind_textdomain_codeset(3)]: https://www.man7.org/linux/man-pages/man3/bind_textdomain_codeset.3.html
///
/// # Panics
///
/// Panics if:
/// * `domainname` or `codeset` contain an internal 0 byte, as such values can't be passed to the
///     underlying C API;
/// * the result is not in UTF-8 (which shouldn't happen as the results should always be ASCII, as
///     they're just codeset names).
pub fn bind_textdomain_codeset<T, U>(domainname: T, codeset: U) -> Result<Option<String>, io::Error>
where
    T: Into<Vec<u8>>,
    U: Into<String>,
{
    let domainname = CString::new(domainname).expect("`domainname` contains an internal 0 byte");
    let codeset = CString::new(codeset.into()).expect("`codeset` contains an internal 0 byte");
    unsafe {
        let result = ffi::bind_textdomain_codeset(domainname.as_ptr(), codeset.as_ptr());
        if result.is_null() {
            let error = io::Error::last_os_error();
            if let Some(0) = error.raw_os_error() {
                return Ok(None);
            } else {
                return Err(error);
            }
        } else {
            let result = CStr::from_ptr(result)
                .to_str()
                .expect("`bind_textdomain_codeset()` returned non-UTF-8 string")
                .to_owned();
            Ok(Some(result))
        }
    }
}

static CONTEXT_SEPARATOR: char = '\x04';

fn build_context_id(ctxt: &str, msgid: &str) -> String {
    format!("{}{}{}", ctxt, CONTEXT_SEPARATOR, msgid)
}

fn panic_on_zero_in_ctxt(msgctxt: &str) {
    if msgctxt.contains('\0') {
        panic!("`msgctxt` contains an internal 0 byte");
    }
}

/// Translate msgid to localized message from the default domain (with context support).
///
/// # Panics
///
/// Panics if:
/// * `msgctxt` or `msgid` contain an internal 0 byte, as such values can't be passed to the
///     underlying C API;
/// * the result is not in UTF-8 (see [this note](./index.html#utf-8-is-required)).
pub fn pgettext<T, U>(msgctxt: T, msgid: U) -> String
where
    T: Into<String>,
    U: Into<String>,
{
    let msgctxt = msgctxt.into();
    panic_on_zero_in_ctxt(&msgctxt);

    let msgid = msgid.into();
    let text = build_context_id(&msgctxt, &msgid);

    let translation = gettext(text);
    if translation.contains(CONTEXT_SEPARATOR as char) {
        return gettext(msgid);
    }

    translation
}

/// Translate msgid to localized message from the default domain (with plural support and context
/// support).
///
/// # Panics
///
/// Panics if:
/// * `msgctxt`, `msgid`, or `msgid_plural` contain an internal 0 byte, as such values can't be
///     passed to the underlying C API;
/// * the result is not in UTF-8 (see [this note](./index.html#utf-8-is-required)).
pub fn npgettext<T, U, V>(msgctxt: T, msgid: U, msgid_plural: V, n: u32) -> String
where
    T: Into<String>,
    U: Into<String>,
    V: Into<String>,
{
    let msgctxt = msgctxt.into();
    panic_on_zero_in_ctxt(&msgctxt);

    let singular_msgid = msgid.into();
    let plural_msgid = msgid_plural.into();
    let singular_ctxt = build_context_id(&msgctxt, &singular_msgid);
    let plural_ctxt = build_context_id(&msgctxt, &plural_msgid);

    let translation = ngettext(singular_ctxt, plural_ctxt, n);
    if translation.contains(CONTEXT_SEPARATOR as char) {
        return ngettext(singular_msgid, plural_msgid, n);
    }

    translation
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn smoke_test() {
        setlocale(LocaleCategory::LcAll, "en_US.UTF-8");

        bindtextdomain("hellorust", "/usr/local/share/locale").unwrap();
        textdomain("hellorust").unwrap();

        assert_eq!("Hello, world!", gettext("Hello, world!"));
    }

    #[test]
    fn plural_test() {
        setlocale(LocaleCategory::LcAll, "en_US.UTF-8");

        bindtextdomain("hellorust", "/usr/local/share/locale").unwrap();
        textdomain("hellorust").unwrap();

        assert_eq!(
            "Hello, world!",
            ngettext("Hello, world!", "Hello, worlds!", 1)
        );
        assert_eq!(
            "Hello, worlds!",
            ngettext("Hello, world!", "Hello, worlds!", 2)
        );
    }

    #[test]
    fn context_test() {
        setlocale(LocaleCategory::LcAll, "en_US.UTF-8");

        bindtextdomain("hellorust", "/usr/local/share/locale").unwrap();
        textdomain("hellorust").unwrap();

        assert_eq!("Hello, world!", pgettext("context", "Hello, world!"));
    }

    #[test]
    fn plural_context_test() {
        setlocale(LocaleCategory::LcAll, "en_US.UTF-8");

        bindtextdomain("hellorust", "/usr/local/share/locale").unwrap();
        textdomain("hellorust").unwrap();

        assert_eq!(
            "Hello, world!",
            npgettext("context", "Hello, world!", "Hello, worlds!", 1)
        );
        assert_eq!(
            "Hello, worlds!",
            npgettext("context", "Hello, world!", "Hello, worlds!", 2)
        );
    }

    #[test]
    #[should_panic(expected = "`msgid` contains an internal 0 byte")]
    fn gettext_panics() {
        gettext("input string\0");
    }

    #[test]
    #[should_panic(expected = "`domainname` contains an internal 0 byte")]
    fn dgettext_panics_on_zero_in_domainname() {
        dgettext("hello\0world!", "hi");
    }

    #[test]
    #[should_panic(expected = "`msgid` contains an internal 0 byte")]
    fn dgettext_panics_on_zero_in_msgid() {
        dgettext("hello world", "another che\0ck");
    }

    #[test]
    #[should_panic(expected = "`domainname` contains an internal 0 byte")]
    fn dcgettext_panics_on_zero_in_domainname() {
        dcgettext("a diff\0erent input", "hello", LocaleCategory::LcAll);
    }

    #[test]
    #[should_panic(expected = "`msgid` contains an internal 0 byte")]
    fn dcgettext_panics_on_zero_in_msgid() {
        dcgettext("world", "yet \0 another\0 one", LocaleCategory::LcMessages);
    }

    #[test]
    #[should_panic(expected = "`msgid` contains an internal 0 byte")]
    fn ngettext_panics_on_zero_in_msgid() {
        ngettext("singular\0form", "plural form", 10);
    }

    #[test]
    #[should_panic(expected = "`msgid_plural` contains an internal 0 byte")]
    fn ngettext_panics_on_zero_in_msgid_plural() {
        ngettext("singular form", "plural\0form", 0);
    }

    #[test]
    #[should_panic(expected = "`domainname` contains an internal 0 byte")]
    fn dngettext_panics_on_zero_in_domainname() {
        dngettext("do\0main", "one", "many", 0);
    }

    #[test]
    #[should_panic(expected = "`msgid` contains an internal 0 byte")]
    fn dngettext_panics_on_zero_in_msgid() {
        dngettext("domain", "just a\0 single one", "many", 100);
    }

    #[test]
    #[should_panic(expected = "`msgid_plural` contains an internal 0 byte")]
    fn dngettext_panics_on_zero_in_msgid_plural() {
        dngettext("d", "1", "many\0many\0many more", 10000);
    }

    #[test]
    #[should_panic(expected = "`domainname` contains an internal 0 byte")]
    fn dcngettext_panics_on_zero_in_domainname() {
        dcngettext(
            "doma\0in",
            "singular",
            "plural",
            42,
            LocaleCategory::LcCType,
        );
    }

    #[test]
    #[should_panic(expected = "`msgid` contains an internal 0 byte")]
    fn dcngettext_panics_on_zero_in_msgid() {
        dcngettext("domain", "\0ne", "plural", 13, LocaleCategory::LcNumeric);
    }

    #[test]
    #[should_panic(expected = "`msgid_plural` contains an internal 0 byte")]
    fn dcngettext_panics_on_zero_in_msgid_plural() {
        dcngettext("d-o-m-a-i-n", "one", "a\0few", 0, LocaleCategory::LcTime);
    }

    #[test]
    #[should_panic(expected = "`domainname` contains an internal 0 byte")]
    fn textdomain_panics_on_zero_in_domainname() {
        textdomain("this is \0 my domain").unwrap();
    }

    #[test]
    #[should_panic(expected = "`domainname` contains an internal 0 byte")]
    fn bindtextdomain_panics_on_zero_in_domainname() {
        bindtextdomain("\0bind this", "/usr/share/locale").unwrap();
    }

    #[test]
    #[should_panic(expected = "`dirname` contains an internal 0 byte")]
    fn bindtextdomain_panics_on_zero_in_dirname() {
        bindtextdomain("my_domain", "/opt/locales\0").unwrap();
    }

    #[test]
    #[should_panic(expected = "`locale` contains an internal 0 byte")]
    fn setlocale_panics_on_zero_in_locale() {
        setlocale(LocaleCategory::LcCollate, "en_\0US");
    }

    #[test]
    #[should_panic(expected = "`domainname` contains an internal 0 byte")]
    fn bind_textdomain_codeset_panics_on_zero_in_domainname() {
        bind_textdomain_codeset("doma\0in", "UTF-8").unwrap();
    }

    #[test]
    #[should_panic(expected = "`codeset` contains an internal 0 byte")]
    fn bind_textdomain_codeset_panics_on_zero_in_codeset() {
        bind_textdomain_codeset("name", "K\0I8-R").unwrap();
    }

    #[test]
    #[should_panic(expected = "`msgctxt` contains an internal 0 byte")]
    fn pgettext_panics_on_zero_in_msgctxt() {
        pgettext("context\0", "string");
    }

    #[test]
    #[should_panic(expected = "`msgid` contains an internal 0 byte")]
    fn pgettext_panics_on_zero_in_msgid() {
        pgettext("ctx", "a message\0to be translated");
    }

    #[test]
    #[should_panic(expected = "`msgctxt` contains an internal 0 byte")]
    fn npgettext_panics_on_zero_in_msgctxt() {
        npgettext("c\0tx", "singular", "plural", 0);
    }

    #[test]
    #[should_panic(expected = "`msgid` contains an internal 0 byte")]
    fn npgettext_panics_on_zero_in_msgid() {
        npgettext("ctx", "sing\0ular", "many many more", 135626);
    }

    #[test]
    #[should_panic(expected = "`msgid_plural` contains an internal 0 byte")]
    fn npgettext_panics_on_zero_in_msgid_plural() {
        npgettext("context", "uno", "one \0fewer", 10585);
    }
}
