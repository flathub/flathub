#[cfg(feature = "quick-xml")]
use quick_xml::de::DeError;
use static_assertions::assert_impl_all;
use std::{convert::Infallible, error, fmt, io, sync::Arc};
use zbus_names::{Error as NamesError, OwnedErrorName};
use zvariant::Error as VariantError;

use crate::{fdo, Message, MessageType};

/// The error type for `zbus`.
///
/// The various errors that can be reported by this crate.
#[derive(Debug)]
#[non_exhaustive]
#[allow(clippy::upper_case_acronyms)]
pub enum Error {
    /// Interface not found
    InterfaceNotFound,
    /// Invalid D-Bus address.
    Address(String),
    /// An I/O error.
    #[deprecated(note = "Use `Error::InputOutput` instead")]
    Io(io::Error),
    /// An I/O error.
    InputOutput(Arc<io::Error>),
    /// Invalid message field.
    InvalidField,
    /// Data too large.
    ExcessData,
    /// A [zvariant](../zvariant/index.html) error.
    Variant(VariantError),
    /// A [zbus_names](../zbus_names/index.html) error.
    Names(NamesError),
    /// Endian signature invalid or doesn't match expectation.
    IncorrectEndian,
    /// Initial handshake error.
    Handshake(String),
    /// Unexpected or incorrect reply.
    InvalidReply,
    /// A D-Bus method error reply.
    // According to the spec, there can be all kinds of details in D-Bus errors but nobody adds
    // anything more than a string description.
    MethodError(OwnedErrorName, Option<String>, Arc<Message>),
    /// A required field is missing in the message headers.
    MissingField,
    /// Invalid D-Bus GUID.
    InvalidGUID,
    /// Unsupported function, or support currently lacking.
    Unsupported,
    /// A [`fdo::Error`] transformed into [`Error`].
    FDO(Box<fdo::Error>),
    #[cfg(feature = "xml")]
    /// An XML error
    SerdeXml(serde_xml_rs::Error),
    #[cfg(feature = "quick-xml")]
    /// An XML error from quick_xml
    QuickXml(DeError),
    NoBodySignature,
    /// The requested name was already claimed by another peer.
    NameTaken,
    /// Invalid [match rule][MR] string.
    ///
    /// [MR]: https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-routing-match-rules
    InvalidMatchRule,
    /// Generic error.
    Failure(String),
    /// A required parameter was missing.
    MissingParameter(&'static str),
}

assert_impl_all!(Error: Send, Sync, Unpin);

impl PartialEq for Error {
    fn eq(&self, other: &Self) -> bool {
        match (self, other) {
            (Self::Address(_), Self::Address(_)) => true,
            (Self::InterfaceNotFound, Self::InterfaceNotFound) => true,
            (Self::Handshake(_), Self::Handshake(_)) => true,
            (Self::InvalidReply, Self::InvalidReply) => true,
            (Self::ExcessData, Self::ExcessData) => true,
            (Self::IncorrectEndian, Self::IncorrectEndian) => true,
            (Self::MethodError(_, _, _), Self::MethodError(_, _, _)) => true,
            (Self::MissingField, Self::MissingField) => true,
            (Self::InvalidGUID, Self::InvalidGUID) => true,
            (Self::Unsupported, Self::Unsupported) => true,
            (Self::FDO(s), Self::FDO(o)) => s == o,
            (Self::NoBodySignature, Self::NoBodySignature) => true,
            (Self::InvalidField, Self::InvalidField) => true,
            (Self::InvalidMatchRule, Self::InvalidMatchRule) => true,
            (Self::Variant(s), Self::Variant(o)) => s == o,
            (Self::Names(s), Self::Names(o)) => s == o,
            (Self::NameTaken, Self::NameTaken) => true,
            #[allow(deprecated)]
            (Error::Io(_), Self::Io(_)) => false,
            (Error::InputOutput(_), Self::InputOutput(_)) => false,
            #[cfg(feature = "xml")]
            (Self::SerdeXml(_), Self::SerdeXml(_)) => false,
            #[cfg(feature = "quick-xml")]
            (Self::QuickXml(_), Self::QuickXml(_)) => false,
            (Self::Failure(s1), Self::Failure(s2)) => s1 == s2,
            (_, _) => false,
        }
    }
}

impl error::Error for Error {
    fn source(&self) -> Option<&(dyn error::Error + 'static)> {
        match self {
            Error::InterfaceNotFound => None,
            Error::Address(_) => None,
            #[allow(deprecated)]
            Error::Io(e) => Some(e),
            Error::InputOutput(e) => Some(e),
            Error::ExcessData => None,
            Error::Handshake(_) => None,
            Error::IncorrectEndian => None,
            Error::Variant(e) => Some(e),
            Error::Names(e) => Some(e),
            Error::InvalidReply => None,
            Error::MethodError(_, _, _) => None,
            Error::InvalidGUID => None,
            Error::Unsupported => None,
            Error::FDO(e) => Some(e),
            #[cfg(feature = "xml")]
            Error::SerdeXml(e) => Some(e),
            #[cfg(feature = "quick-xml")]
            Error::QuickXml(e) => Some(e),
            Error::NoBodySignature => None,
            Error::InvalidField => None,
            Error::MissingField => None,
            Error::NameTaken => None,
            Error::InvalidMatchRule => None,
            Error::Failure(_) => None,
            Error::MissingParameter(_) => None,
        }
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Error::InterfaceNotFound => write!(f, "Interface not found"),
            Error::Address(e) => write!(f, "address error: {e}"),
            Error::ExcessData => write!(f, "excess data"),
            #[allow(deprecated)]
            Error::Io(e) => write!(f, "I/O error: {e}"),
            Error::InputOutput(e) => write!(f, "I/O error: {e}"),
            Error::Handshake(e) => write!(f, "D-Bus handshake failed: {e}"),
            Error::IncorrectEndian => write!(f, "incorrect endian"),
            Error::InvalidField => write!(f, "invalid message field"),
            Error::Variant(e) => write!(f, "{e}"),
            Error::Names(e) => write!(f, "{e}"),
            Error::InvalidReply => write!(f, "Invalid D-Bus method reply"),
            Error::MissingField => write!(f, "A required field is missing from message headers"),
            Error::MethodError(name, detail, _reply) => write!(
                f,
                "{}: {}",
                **name,
                detail.as_ref().map(|s| s.as_str()).unwrap_or("no details")
            ),
            Error::InvalidGUID => write!(f, "Invalid GUID"),
            Error::Unsupported => write!(f, "Connection support is lacking"),
            Error::FDO(e) => write!(f, "{e}"),
            #[cfg(feature = "xml")]
            Error::SerdeXml(e) => write!(f, "XML error: {}", e),
            #[cfg(feature = "quick-xml")]
            Error::QuickXml(e) => write!(f, "XML error: {e}"),
            Error::NoBodySignature => write!(f, "missing body signature in the message"),
            Error::NameTaken => write!(f, "name already taken on the bus"),
            Error::InvalidMatchRule => write!(f, "Invalid match rule string"),
            Error::Failure(e) => write!(f, "{e}"),
            Error::MissingParameter(p) => {
                write!(f, "Parameter `{}` was not specified but it is required", p)
            }
        }
    }
}

impl Clone for Error {
    fn clone(&self) -> Self {
        match self {
            Error::InterfaceNotFound => Error::InterfaceNotFound,
            Error::Address(e) => Error::Address(e.clone()),
            Error::ExcessData => Error::ExcessData,
            #[allow(deprecated)]
            Error::Io(e) => Error::Io(io::Error::new(e.kind(), e.to_string())),
            Error::InputOutput(e) => Error::InputOutput(e.clone()),
            Error::Handshake(e) => Error::Handshake(e.clone()),
            Error::IncorrectEndian => Error::IncorrectEndian,
            Error::InvalidField => Error::InvalidField,
            Error::Variant(e) => Error::Variant(e.clone()),
            Error::Names(e) => Error::Names(e.clone()),
            Error::InvalidReply => Error::InvalidReply,
            Error::MissingField => Error::MissingField,
            Error::MethodError(name, detail, reply) => {
                Error::MethodError(name.clone(), detail.clone(), reply.clone())
            }
            Error::InvalidGUID => Error::InvalidGUID,
            Error::Unsupported => Error::Unsupported,
            Error::FDO(e) => Error::FDO(e.clone()),
            #[cfg(feature = "xml")]
            Error::SerdeXml(e) => Error::Failure(e.to_string()),
            #[cfg(feature = "quick-xml")]
            // Until https://github.com/tafia/quick-xml/pull/521 is merged and released.
            Error::QuickXml(e) => Error::QuickXml(e.clone()),
            Error::NoBodySignature => Error::NoBodySignature,
            Error::NameTaken => Error::NameTaken,
            Error::InvalidMatchRule => Error::InvalidMatchRule,
            Error::Failure(e) => Error::Failure(e.clone()),
            Error::MissingParameter(p) => Error::MissingParameter(p),
        }
    }
}

impl From<io::Error> for Error {
    fn from(val: io::Error) -> Self {
        Error::InputOutput(Arc::new(val))
    }
}

#[cfg(unix)]
impl From<nix::Error> for Error {
    fn from(val: nix::Error) -> Self {
        io::Error::from_raw_os_error(val as i32).into()
    }
}

impl From<VariantError> for Error {
    fn from(val: VariantError) -> Self {
        Error::Variant(val)
    }
}

impl From<NamesError> for Error {
    fn from(val: NamesError) -> Self {
        match val {
            NamesError::Variant(e) => Error::Variant(e),
            e => Error::Names(e),
        }
    }
}

impl From<fdo::Error> for Error {
    fn from(val: fdo::Error) -> Self {
        match val {
            fdo::Error::ZBus(e) => e,
            e => Error::FDO(Box::new(e)),
        }
    }
}

#[cfg(feature = "xml")]
impl From<serde_xml_rs::Error> for Error {
    fn from(val: serde_xml_rs::Error) -> Self {
        Error::SerdeXml(val)
    }
}

#[cfg(feature = "quick-xml")]
impl From<DeError> for Error {
    fn from(val: DeError) -> Self {
        Error::QuickXml(val)
    }
}

impl From<Infallible> for Error {
    fn from(i: Infallible) -> Self {
        match i {}
    }
}

// For messages that are D-Bus error returns
impl From<Message> for Error {
    fn from(message: Message) -> Error {
        Self::from(Arc::new(message))
    }
}

impl From<Arc<Message>> for Error {
    fn from(message: Arc<Message>) -> Error {
        // FIXME: Instead of checking this, we should have Method as trait and specific types for
        // each message type.
        let header = match message.header() {
            Ok(header) => header,
            Err(e) => {
                return e;
            }
        };
        if header.primary().msg_type() != MessageType::Error {
            return Error::InvalidReply;
        }

        if let Ok(Some(name)) = header.error_name() {
            let name = name.to_owned().into();
            match message.body_unchecked::<&str>() {
                Ok(detail) => Error::MethodError(name, Some(String::from(detail)), message),
                Err(_) => Error::MethodError(name, None, message),
            }
        } else {
            Error::InvalidReply
        }
    }
}

/// Alias for a `Result` with the error type `zbus::Error`.
pub type Result<T> = std::result::Result<T, Error>;
