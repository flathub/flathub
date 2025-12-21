use proc_macro2::TokenStream;
use proc_macro_crate::{crate_name, FoundCrate};
use quote::{format_ident, quote};
use syn::{Attribute, FnArg, Ident, Pat, PatIdent, PatType};

pub fn zbus_path() -> TokenStream {
    if let Ok(FoundCrate::Name(name)) = crate_name("zbus") {
        let ident = format_ident!("{}", name);
        quote! { ::#ident }
    } else {
        quote! { ::zbus }
    }
}

pub fn typed_arg(arg: &FnArg) -> Option<&PatType> {
    match arg {
        FnArg::Typed(t) => Some(t),
        _ => None,
    }
}

pub fn pat_ident(pat: &PatType) -> Option<&Ident> {
    match &*pat.pat {
        Pat::Ident(PatIdent { ident, .. }) => Some(ident),
        _ => None,
    }
}

pub fn get_doc_attrs(attrs: &[Attribute]) -> Vec<&Attribute> {
    attrs.iter().filter(|x| x.path.is_ident("doc")).collect()
}

// Convert to pascal case, assuming snake case.
// If `s` is already in pascal case, should yield the same result.
pub fn pascal_case(s: &str) -> String {
    let mut pascal = String::new();
    let mut capitalize = true;
    for ch in s.chars() {
        if ch == '_' {
            capitalize = true;
        } else if capitalize {
            pascal.push(ch.to_ascii_uppercase());
            capitalize = false;
        } else {
            pascal.push(ch);
        }
    }
    pascal
}

pub fn is_blank(s: &str) -> bool {
    s.trim().is_empty()
}
