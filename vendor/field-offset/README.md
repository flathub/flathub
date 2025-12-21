# field-offset: safe pointer-to-member functionality

This crate implements an `offset_of!(...)` macro which safely encapsulates
a pointer-to-member.

Example:
```rust
struct Foo {
    x: u32,
    y: f64
}

let foo_y = offset_of!(Foo => y);

let mut a = Foo { x: 1, y: 2.0 };

*foo_y.apply_mut(&mut a) = 3.0;

assert!(a.y == 3.0);
```

The macro returns an instance of `FieldOffset<T, U>`, which represents a
pointer to a field of type `U` within a containing type, `T`.

The `FieldOffset` type implements `Add`. Applying the resulting field offset
is equivalent to applying the first field offset, then applying the second
field offset.

The macro also supports accessing nested fields:

```rust
let bar_foo_y = offset_of!(Bar => foo: Foo => y);
```

