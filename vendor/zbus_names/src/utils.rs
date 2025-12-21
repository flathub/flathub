macro_rules! impl_try_from {
    (ty: $type:ty, owned_ty: $owned_type:ty, validate_fn: $validate_fn:ident, try_from: [$($from:ty),*],) => {
        $(
            impl<'s> TryFrom<$from> for $type {
                type Error = Error;

                fn try_from(value: $from) -> Result<Self> {
                    let value = Str::from(value);
                    $validate_fn(value.as_str())?;
                    Ok(Self(value))
                }
            }

            impl<'s> TryFrom<$from> for $owned_type {
                type Error = Error;

                fn try_from(value: $from) -> Result<Self> {
                    Ok(Self::from(<$type>::try_from(value)?))
                }
            }
        )*
    };
}

pub(crate) use impl_try_from;
