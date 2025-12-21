# gettext-rs

Safe bindings for gettext. Please see
[documentation](https://docs.rs/gettext-rs) for details.

## Licensing

This crate depends on `gettext-sys`, which compiles GNU gettext on platforms
that don't have a native gettext implementation. GNU gettext is licensed under
LGPL, and is linked statically, which means **you have to abide by LGPL**. If
you don't want or can't do that, there are two ways out:

1. if you use glibc or musl libc, enable `gettext-system` feature (see below);
2. dynamically link to GNU gettext library you obtained by some other means,
   like a package manager. For details, see environment variables in
   `gettext-sys` documentation.

## Usage

(If you know how to use gettext and just want a gist of this crate's API, [skip
to the next section](#complete-api-example)).

To internationalize your program with gettext, you have to do four things:

1. wrap translatable strings into calls to appropriate gettext functions;
2. use tools to extract those strings into a so-called "PO template file";
3. translate the strings in the template, getting a so-called "message catalog"
   as a result;
4. compile the message catalog from the plain-text, human-readable PO format to
   the binary MO format, and install them into a well-known location like
   _/usr/local/share/locale_.

This crate only covers the first step, the markup. To extract messages, use the
xgettext program (from GNU gettext version 0.24 or newer) or `xtr` (`cargo
install xtr`). To translate, you can use desktop tools like [Poedit][], sites
like [Crowdin][], or any text editor. To compile from PO to MO, use `msgfmt`
tool from GNU gettext. The way you install files highly depend on your
distribution method, so it's not covered here either.

[Poedit]: https://poedit.net
[Crowdin]: https://crowdin.com

The best resource on gettext is [GNU gettext manual][]. This crate has the same
API, so you should have an easy time transferring the advice from that manual to
this crate. In a pitch, you can also glance at the manpages for C functions
which this crate wraps.

[GNU gettext manual]: https://www.gnu.org/software/gettext/manual/index.html

## Complete API example

```rust
use gettextrs::*;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Specify the name of the .mo file to use.
    textdomain("hellorust")?;
    // Ask gettext for UTF-8 strings. THIS CRATE CAN'T HANDLE NON-UTF-8 DATA!
    bind_textdomain_codeset("hellorust", "UTF-8")?;

    // You could also use `TextDomain` builder which calls `textdomain` and
    // other functions for you:
    //
    // TextDomain::new("hellorust").init()?;

    // `gettext()` simultaneously marks a string for translation and translates
    // it at runtime.
    println!("Translated: {}", gettext("Hello, world!"));

    // gettext supports plurals, i.e. you can have different messages depending
    // on the number of items the message mentions. This even works for
    // languages that have more than one plural form, like Russian or Czech.
    println!("Singular: {}", ngettext("One thing", "Multiple things", 1));
    println!("Plural: {}", ngettext("One thing", "Multiple things", 2));

    // gettext de-duplicates strings, i.e. the same string used multiple times
    // will have a single entry in the PO and MO files. However, the same words
    // might have different meaning depending on the context. To distinguish
    // between different contexts, gettext accepts an additional string:
    println!("With context: {}", pgettext("This is the context", "Hello, world!"));
    println!(
        "Plural with context: {}",
        npgettext("This is the context", "One thing", "Multiple things", 2));

    Ok(())
}
```

## Features

- `gettext-system`: if enabled, _asks_ the crate to use the gettext
    implementation that's part of glibc or musl libc. This only works on:

    * Linux with glibc or musl libc;
    * Windows + GNU (e.g. [MSYS2](https://www.msys2.org/)) with `gettext-devel`
        installed e.g. using:

        ```
        pacman --noconfirm -S base-devel mingw-w64-x86_64-gcc libxml2-devel tar
        ```
    * GNU/Hurd with glibc;

    If none of those conditions hold, the crate will proceed to building and
    statically linking its own copy of GNU gettext!

    This enables `gettext-system` feature of the underlying `gettext-sys` crate.

## Environment variables

This crate doesn't use any. See also the documentation for the underlying
`gettext-sys` crate.
