# Rust Graphene bindings

__Rust__ bindings and wrappers for [__Graphene__](https://github.com/ebassi/graphene), part of [gtk-rs-core](https://github.com/gtk-rs/gtk-rs-core).

Graphene __1.10__ is the lowest supported version for the underlying library.

## Minimum supported Rust version

Currently, the minimum supported Rust version is `1.83.0`.

## Documentation

 * [Rust API - Stable](https://gtk-rs.org/gtk-rs-core/stable/latest/docs/graphene/)
 * [Rust API - Development](https://gtk-rs.org/gtk-rs-core/git/docs/graphene)
 * [C API](https://ebassi.github.io/graphene/docs/)
 * [GTK Installation instructions](https://www.gtk.org/docs/installations/)

## Using

We recommend using [crates from crates.io](https://crates.io/keywords/gtk-rs),
as [demonstrated here](https://gtk-rs.org/#using).

If you want to track the bleeding edge, use the git dependency instead:

```toml
[dependencies]
graphene = { git = "https://github.com/gtk-rs/gtk-rs-core.git", package = "graphene" }
```

Avoid mixing versioned and git crates like this:

```toml
# This will not compile
[dependencies]
graphene = "0.13"
graphene = { git = "https://github.com/gtk-rs/gtk-rs-core.git", package = "graphene" }
```

### See Also

 * [glib](https://crates.io/crates/glib)

## License

__graphene__ is available under the MIT License, please refer to it.
