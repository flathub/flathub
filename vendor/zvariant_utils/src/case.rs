//! Contains utilities used to convert strings between different cases.

/// Convert to pascal or camel case, assuming snake case.
///
/// If `s` is already in pascal or camel case, should yield the same result.
pub fn pascal_or_camel_case(s: &str, is_pascal_case: bool) -> String {
    let mut result = String::new();
    let mut capitalize = is_pascal_case;
    let mut first = true;
    for ch in s.chars() {
        if ch == '_' {
            capitalize = true;
        } else if capitalize {
            result.push(ch.to_ascii_uppercase());
            capitalize = false;
        } else if first && !is_pascal_case {
            result.push(ch.to_ascii_lowercase());
        } else {
            result.push(ch);
        }

        if first {
            first = false;
        }
    }
    result
}

/// Convert to snake case, assuming pascal case.
///
/// If `s` is already in snake case, should yield the same result.
pub fn snake_case(s: &str) -> String {
    let mut snake = String::new();
    for ch in s.chars() {
        if ch.is_ascii_uppercase() && !snake.is_empty() {
            snake.push('_');
        }
        snake.push(ch.to_ascii_lowercase());
    }
    snake
}
