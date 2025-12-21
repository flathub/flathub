use syn::{
    spanned::Spanned, Attribute, Lit, LitBool, LitStr, Meta, MetaList, NestedMeta, Result, Type,
    TypePath,
};

// find the #[@attr_name] attribute in @attrs
fn find_attribute_meta(attrs: &[Attribute], attr_name: &str) -> Result<Option<MetaList>> {
    let meta = match attrs.iter().find(|a| a.path.is_ident(attr_name)) {
        Some(a) => a.parse_meta(),
        _ => return Ok(None),
    }?;
    match meta {
        Meta::List(n) => Ok(Some(n)),
        _ => Err(syn::Error::new(
            meta.span(),
            format!("{attr_name} meta must specify a meta list"),
        )),
    }
}

fn get_meta_value<'a>(meta: &'a Meta, attr: &str) -> Result<&'a Lit> {
    match meta {
        Meta::NameValue(meta) => Ok(&meta.lit),
        Meta::Path(_) => Err(syn::Error::new(
            meta.span(),
            format!("attribute `{attr}` must have a value"),
        )),
        Meta::List(_) => Err(syn::Error::new(
            meta.span(),
            format!("attribute {attr} is not a list"),
        )),
    }
}

/// Compares `ident` and `attr` and in case they match ensures `value` is `Some` and contains a
/// [`struct@LitStr`]. Returns `true` in case `ident` and `attr` match, otherwise false.
///
/// # Errors
///
/// Returns an error in case `ident` and `attr` match but the value is not `Some` or is not a
/// [`struct@LitStr`].
pub fn match_attribute_with_str_value<'a>(
    meta: &'a Meta,
    attr: &str,
) -> Result<Option<&'a LitStr>> {
    if meta.path().is_ident(attr) {
        match get_meta_value(meta, attr)? {
            Lit::Str(value) => Ok(Some(value)),
            _ => Err(syn::Error::new(
                meta.span(),
                format!("value of the `{attr}` attribute must be a string literal"),
            )),
        }
    } else {
        Ok(None)
    }
}

/// Compares `ident` and `attr` and in case they match ensures `value` is `Some` and contains a
/// [`struct@LitBool`]. Returns `true` in case `ident` and `attr` match, otherwise false.
///
/// # Errors
///
/// Returns an error in case `ident` and `attr` match but the value is not `Some` or is not a
/// [`struct@LitBool`].
pub fn match_attribute_with_bool_value<'a>(
    meta: &'a Meta,
    attr: &str,
) -> Result<Option<&'a LitBool>> {
    if meta.path().is_ident(attr) {
        match get_meta_value(meta, attr)? {
            Lit::Bool(value) => Ok(Some(value)),
            other => Err(syn::Error::new(
                other.span(),
                format!("value of the `{attr}` attribute must be a boolean literal"),
            )),
        }
    } else {
        Ok(None)
    }
}

pub fn match_attribute_with_str_list_value(meta: &Meta, attr: &str) -> Result<Option<Vec<String>>> {
    if meta.path().is_ident(attr) {
        match meta {
            Meta::List(list) => {
                let mut values = Vec::with_capacity(list.nested.len());

                for meta in &list.nested {
                    values.push(match meta {
                        NestedMeta::Lit(Lit::Str(lit)) => Ok(lit.value()),
                        NestedMeta::Lit(lit) => Err(syn::Error::new(
                            lit.span(),
                            format!("invalid literal type for `{attr}` attribute"),
                        )),
                        NestedMeta::Meta(meta) => Err(syn::Error::new(
                            meta.span(),
                            format!("`{attr}` attribute must be a list of string literals"),
                        )),
                    }?)
                }

                Ok(Some(values))
            }
            _ => Err(syn::Error::new(
                meta.span(),
                format!("invalid meta type for attribute `{attr}`"),
            )),
        }
    } else {
        Ok(None)
    }
}

/// Compares `ident` and `attr` and in case they match ensures `value` is `None`. Returns `true` in
/// case `ident` and `attr` match, otherwise false.
///
/// # Errors
///
/// Returns an error in case `ident` and `attr` match but the value is not `None`.
pub fn match_attribute_without_value(meta: &Meta, attr: &str) -> Result<bool> {
    if meta.path().is_ident(attr) {
        match meta {
            Meta::Path(_) => Ok(true),
            Meta::List(_) => Err(syn::Error::new(
                meta.span(),
                format!("attribute {attr} is not a list"),
            )),
            Meta::NameValue(_) => Err(syn::Error::new(
                meta.span(),
                format!("attribute `{attr}` must not have a value"),
            )),
        }
    } else {
        Ok(false)
    }
}

/// Returns an iterator over the contents of all [`MetaList`]s with the specified identifier in an
/// array of [`Attribute`]s.
pub fn iter_meta_lists(
    attrs: &[Attribute],
    list_name: &str,
) -> Result<impl Iterator<Item = NestedMeta>> {
    let meta = find_attribute_meta(attrs, list_name)?;

    Ok(meta.into_iter().flat_map(|meta| meta.nested.into_iter()))
}

/// Generates one or more structures used for parsing attributes in proc macros.
///
/// Generated structures have one static method called parse that accepts a slice of [`Attribute`]s.
/// The method finds attributes that contain meta lists (look like `#[your_custom_ident(...)]`) and
/// fills a newly allocated structure with values of the attributes if any.
///
/// The expected input looks as follows:
///
/// ```
/// # use zvariant_utils::def_attrs;
/// def_attrs! {
///     crate zvariant;
///
///     /// A comment.
///     pub StructAttributes("struct") { foo str, bar str, baz none };
///     #[derive(Hash)]
///     FieldAttributes("field") { field_attr bool };
/// }
/// ```
///
/// Here we see multiple entries: an entry for an attributes group called `StructAttributes` and
/// another one for `FieldAttributes`. The former has three defined attributes: `foo`, `bar` and
/// `baz`. The generated structures will look like this in that case:
///
/// ```
/// /// A comment.
/// #[derive(Default, Clone, Debug)]
/// pub struct StructAttributes {
///     foo: Option<String>,
///     bar: Option<String>,
///     baz: bool,
/// }
///
/// #[derive(Hash)]
/// #[derive(Default, Clone, Debug)]
/// struct FieldAttributes {
///     field_attr: Option<bool>,
/// }
/// ```
///
/// `foo` and `bar` attributes got translated to fields with `Option<String>` type which contain the
/// value of the attribute when one is specified. They are marked with `str` keyword which stands
/// for string literals. The `baz` attribute, on the other hand, has `bool` type because it's an
/// attribute without value marked by the `none` keyword.
///
/// Currently the following literals are supported:
///
/// * `str` - string literals;
/// * `bool` - boolean literals;
/// * `[str]` - lists of string literals (`#[macro_name(foo("bar", "baz"))]`);
/// * `none` - no literal at all, the attribute is specified alone.
///
/// The strings between braces are embedded into error messages produced when an attribute defined
/// for one attribute group is used on another group where it is not defined. For example, if the
/// `field_attr` attribute was encountered by the generated `StructAttributes::parse` method, the
/// error message would say that it "is not allowed on structs".
///
/// # Nested attribute lists
///
/// It is possible to create nested lists for specific attributes. This is done as follows:
///
/// ```
/// # use zvariant_utils::def_attrs;
/// def_attrs! {
///     crate zvariant;
///
///     pub OuterAttributes("outer") {
///         simple_attr bool,
///         nested_attr {
///             /// An example of nested attributes.
///             pub InnerAttributes("inner") {
///                 inner_attr str
///             }
///         }
///     };
/// }
/// ```
///
/// The syntax for inner attributes is the same as for the outer attributes, but you can specify
/// only one inner attribute per outer attribute.
///
/// # Calling the macro multiple times
///
/// The macro generates an array called `ALLOWED_ATTRS` that contains a list of allowed attributes.
/// Calling the macro twice in the same scope will cause a name alias and thus will fail to compile.
/// You need to place each macro invocation into a module in that case.
///
/// # Errors
///
/// The generated parse method checks for some error conditions:
///
/// 1. Unknown attributes. When multiple attribute groups are defined in the same macro invocation,
/// one gets a different error message when providing an attribute from a different attribute group.
/// 2. Duplicate attributes.
/// 3. Missing attribute value or present attribute value when none is expected.
/// 4. Invalid literal type for attributes with values.
#[macro_export]
macro_rules! def_attrs {
    (@attr_ty str) => {::std::option::Option<::std::string::String>};
    (@attr_ty bool) => {::std::option::Option<bool>};
    (@attr_ty [str]) => {::std::option::Option<::std::vec::Vec<::std::string::String>>};
    (@attr_ty none) => {bool};
    (@attr_ty {
        $(#[$m:meta])*
        $vis:vis $name:ident($what:literal) {
            $($attr_name:ident $kind:tt),+
        }
    }) => {::std::option::Option<$name>};
    (@match_attr_with $attr_name:ident, $meta:ident, $self:ident, $matched:expr) => {
        if let ::std::option::Option::Some(value) = $matched? {
            if $self.$attr_name.is_none() {
                $self.$attr_name = ::std::option::Option::Some(value.value());
                return Ok(());
            } else {
                return ::std::result::Result::Err(::syn::Error::new(
                    $meta.span(),
                    ::std::concat!("duplicate `", ::std::stringify!($attr_name), "` attribute")
                ));
            }
        }
    };
    (@match_attr str $attr_name:ident, $meta:ident, $self:ident) => {
        $crate::def_attrs!(
            @match_attr_with
            $attr_name,
            $meta,
            $self,
            $crate::macros::match_attribute_with_str_value(
                $meta,
                ::std::stringify!($attr_name),
            )
        )
    };
    (@match_attr bool $attr_name:ident, $meta:ident, $self:ident) => {
        $crate::def_attrs!(
            @match_attr_with
            $attr_name,
            $meta,
            $self,
            $crate::macros::match_attribute_with_bool_value(
                $meta,
                ::std::stringify!($attr_name),
            )
        )
    };
    (@match_attr [str] $attr_name:ident, $meta:ident, $self:ident) => {
        if let Some(list) = $crate::macros::match_attribute_with_str_list_value(
            $meta,
            ::std::stringify!($attr_name),
        )? {
            if $self.$attr_name.is_none() {
                $self.$attr_name = Some(list);
                return Ok(());
            } else {
                return ::std::result::Result::Err(::syn::Error::new(
                    $meta.span(),
                    concat!("duplicate `", stringify!($attr_name), "` attribute")
                ));
            }
        }
    };
    (@match_attr none $attr_name:ident, $meta:ident, $self:ident) => {
        if $crate::macros::match_attribute_without_value(
            $meta,
            ::std::stringify!($attr_name),
        )? {
            if !$self.$attr_name {
                $self.$attr_name = true;
                return Ok(());
            } else {
                return ::std::result::Result::Err(::syn::Error::new(
                    $meta.span(),
                    concat!("duplicate `", stringify!($attr_name), "` attribute")
                ));
            }
        }
    };
    (@match_attr {
        $(#[$m:meta])*
        $vis:vis $name:ident($what:literal) $body:tt
    } $attr_name:ident, $meta:expr, $self:ident) => {
        if $meta.path().is_ident(::std::stringify!($attr_name)) {
            return if $self.$attr_name.is_none() {
                match $meta {
                    ::syn::Meta::List(meta) => {
                        $self.$attr_name = ::std::option::Option::Some($name::parse_nested_metas(
                            meta.nested.iter()
                        )?);
                        ::std::result::Result::Ok(())
                    }
                    ::syn::Meta::Path(_) => {
                        $self.$attr_name = ::std::option::Option::Some($name::default());
                        ::std::result::Result::Ok(())
                    }
                    ::syn::Meta::NameValue(_) => Err(::syn::Error::new(
                        $meta.span(),
                        ::std::format!(::std::concat!(
                            "attribute `", ::std::stringify!($attr_name),
                            "` must be either a list or a path"
                        )),
                    ))
                }
            } else {
                ::std::result::Result::Err(::syn::Error::new(
                    $meta.span(),
                    concat!("duplicate `", stringify!($attr_name), "` attribute")
                ))
            }
        }
    };
    (@def_ty $list_name:ident str) => {};
    (@def_ty $list_name:ident bool) => {};
    (@def_ty $list_name:ident [str]) => {};
    (@def_ty $list_name:ident none) => {};
    (
        @def_ty $list_name:ident {
            $(#[$m:meta])*
            $vis:vis $name:ident($what:literal) {
                $($attr_name:ident $kind:tt),+
            }
        }
    ) => {
        // Recurse further to potentially define nested lists.
        $($crate::def_attrs!(@def_ty $attr_name $kind);)+

        $crate::def_attrs!(
            @def_struct
            $list_name
            $(#[$m])*
            $vis $name($what) {
                $($attr_name $kind),+
            }
        );
    };
    (
        @def_struct
        $list_name:ident
        $(#[$m:meta])*
        $vis:vis $name:ident($what:literal) {
            $($attr_name:ident $kind:tt),+
        }
    ) => {
        $(#[$m])*
        #[derive(Default, Clone, Debug)]
        $vis struct $name {
            $(pub $attr_name: $crate::def_attrs!(@attr_ty $kind)),+
        }

        impl $name {
            pub fn parse_meta(
                &mut self,
                meta: &::syn::Meta
            ) -> ::syn::Result<()> {
                use ::syn::spanned::Spanned;

                // This creates subsequent if blocks for simplicity. Any block that is taken
                // either returns an error or sets the attribute field and returns success.
                $(
                    $crate::def_attrs!(@match_attr $kind $attr_name, meta, self);
                )+

                // None of the if blocks have been taken, return the appropriate error.
                let is_valid_attr = ALLOWED_ATTRS.iter().any(|attr| meta.path().is_ident(attr));
                return ::std::result::Result::Err(::syn::Error::new(meta.span(), if is_valid_attr {
                    ::std::format!(
                        ::std::concat!("attribute `{}` is not allowed on ", $what),
                        meta.path().get_ident().unwrap()
                    )
                } else {
                    ::std::format!("unknown attribute `{}`", meta.path().get_ident().unwrap())
                }))
            }

            pub fn parse_nested_metas<'a, I>(iter: I) -> syn::Result<Self>
            where
                I: ::std::iter::IntoIterator<Item=&'a ::syn::NestedMeta>
            {
                let mut parsed = $name::default();
                for nested_meta in iter {
                    match nested_meta {
                        ::syn::NestedMeta::Meta(meta) => parsed.parse_meta(meta),
                        ::syn::NestedMeta::Lit(lit) => {
                            ::std::result::Result::Err(::syn::Error::new(
                                lit.span(),
                                ::std::concat!(
                                    "attribute `", ::std::stringify!($list_name),
                                    "` does not support literals in meta lists"
                                )
                            ))
                        }
                    }?;
                }

                Ok(parsed)
            }

            pub fn parse(attrs: &[::syn::Attribute]) -> ::syn::Result<Self> {
                let mut parsed = $name::default();
                for nested_meta in $crate::macros::iter_meta_lists(attrs, ::std::stringify!($list_name))? {
                    match &nested_meta {
                        ::syn::NestedMeta::Meta(meta) => parsed.parse_meta(meta),
                        ::syn::NestedMeta::Lit(lit) => {
                            ::std::result::Result::Err(::syn::Error::new(
                                lit.span(),
                                ::std::concat!(
                                    "attribute `", ::std::stringify!($list_name),
                                    "` does not support literals in meta lists"
                                )
                            ))
                        }
                    }?;
                }

                Ok(parsed)
            }
        }
    };
    (
        crate $list_name:ident;
        $(
            $(#[$m:meta])*
            $vis:vis $name:ident($what:literal) {
                $($attr_name:ident $kind:tt),+
            }
        );+;
    ) => {
        static ALLOWED_ATTRS: &[&'static str] = &[
            $($(::std::stringify!($attr_name),)+)+
        ];

        $(
            $crate::def_attrs!(
                @def_ty
                $list_name {
                    $(#[$m])*
                    $vis $name($what) {
                        $($attr_name $kind),+
                    }
                }
            );
        )+
    }
}

/// Checks if a [`Type`]'s identifier is "Option".
pub fn ty_is_option(ty: &Type) -> bool {
    match ty {
        Type::Path(TypePath {
            path: syn::Path { segments, .. },
            ..
        }) => segments.last().unwrap().ident == "Option",
        _ => false,
    }
}
