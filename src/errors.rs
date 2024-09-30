use std::fmt;

pub enum SysDManagerErrors {
    GTKBuilderObjectNotfound(String),
}

impl fmt::Debug for SysDManagerErrors {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            SysDManagerErrors::GTKBuilderObjectNotfound(s) => write!(f, "Couldn't find id '{s}'"),
        }
    }
}
