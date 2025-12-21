use std::{
    convert::TryInto,
    io::{Cursor, Write},
};

#[cfg(unix)]
use crate::Fds;
#[cfg(unix)]
use std::{
    os::unix::io::RawFd,
    sync::{Arc, RwLock},
};

use enumflags2::BitFlags;
use zbus_names::{BusName, ErrorName, InterfaceName, MemberName, UniqueName};

use crate::{
    utils::padding_for_8_bytes,
    zvariant::{DynamicType, EncodingContext, ObjectPath, Signature},
    Error, Message, MessageField, MessageFieldCode, MessageFields, MessageFlags, MessageHeader,
    MessagePrimaryHeader, MessageSequence, MessageType, QuickMessageFields, Result,
    MAX_MESSAGE_SIZE,
};

#[cfg(unix)]
type BuildGenericResult = Vec<RawFd>;

#[cfg(not(unix))]
type BuildGenericResult = ();

macro_rules! dbus_context {
    ($n_bytes_before: expr) => {
        EncodingContext::<byteorder::NativeEndian>::new_dbus($n_bytes_before)
    };
}

/// A builder for [`Message`]
#[derive(Debug, Clone)]
pub struct MessageBuilder<'a> {
    header: MessageHeader<'a>,
}

impl<'a> MessageBuilder<'a> {
    fn new(msg_type: MessageType) -> Self {
        let primary = MessagePrimaryHeader::new(msg_type, 0);
        let fields = MessageFields::new();
        let header = MessageHeader::new(primary, fields);
        Self { header }
    }

    /// Create a message of type [`MessageType::MethodCall`].
    pub fn method_call<'p: 'a, 'm: 'a, P, M>(path: P, method_name: M) -> Result<Self>
    where
        P: TryInto<ObjectPath<'p>>,
        M: TryInto<MemberName<'m>>,
        P::Error: Into<Error>,
        M::Error: Into<Error>,
    {
        Self::new(MessageType::MethodCall)
            .path(path)?
            .member(method_name)
    }

    /// Create a message of type [`MessageType::Signal`].
    pub fn signal<'p: 'a, 'i: 'a, 'm: 'a, P, I, M>(path: P, interface: I, name: M) -> Result<Self>
    where
        P: TryInto<ObjectPath<'p>>,
        I: TryInto<InterfaceName<'i>>,
        M: TryInto<MemberName<'m>>,
        P::Error: Into<Error>,
        I::Error: Into<Error>,
        M::Error: Into<Error>,
    {
        Self::new(MessageType::Signal)
            .path(path)?
            .interface(interface)?
            .member(name)
    }

    /// Create a message of type [`MessageType::MethodReturn`].
    pub fn method_return(reply_to: &MessageHeader<'_>) -> Result<Self> {
        Self::new(MessageType::MethodReturn).reply_to(reply_to)
    }

    /// Create a message of type [`MessageType::Error`].
    pub fn error<'e: 'a, E>(reply_to: &MessageHeader<'_>, name: E) -> Result<Self>
    where
        E: TryInto<ErrorName<'e>>,
        E::Error: Into<Error>,
    {
        Self::new(MessageType::Error)
            .error_name(name)?
            .reply_to(reply_to)
    }

    /// Add flags to the message.
    ///
    /// See [`MessageFlags`] documentation for the meaning of the flags.
    ///
    /// The function will return an error if invalid flags are given for the message type.
    pub fn with_flags(mut self, flag: MessageFlags) -> Result<Self> {
        if self.header.message_type()? != MessageType::MethodCall
            && BitFlags::from_flag(flag).contains(MessageFlags::NoReplyExpected)
        {
            return Err(Error::InvalidField);
        }
        let flags = self.header.primary().flags() | flag;
        self.header.primary_mut().set_flags(flags);
        Ok(self)
    }

    /// Set the unique name of the sending connection.
    pub fn sender<'s: 'a, S>(mut self, sender: S) -> Result<Self>
    where
        S: TryInto<UniqueName<'s>>,
        S::Error: Into<Error>,
    {
        self.header
            .fields_mut()
            .replace(MessageField::Sender(sender.try_into().map_err(Into::into)?));
        Ok(self)
    }

    /// Set the object to send a call to, or the object a signal is emitted from.
    pub fn path<'p: 'a, P>(mut self, path: P) -> Result<Self>
    where
        P: TryInto<ObjectPath<'p>>,
        P::Error: Into<Error>,
    {
        self.header
            .fields_mut()
            .replace(MessageField::Path(path.try_into().map_err(Into::into)?));
        Ok(self)
    }

    /// Set the interface to invoke a method call on, or that a signal is emitted from.
    pub fn interface<'i: 'a, I>(mut self, interface: I) -> Result<Self>
    where
        I: TryInto<InterfaceName<'i>>,
        I::Error: Into<Error>,
    {
        self.header.fields_mut().replace(MessageField::Interface(
            interface.try_into().map_err(Into::into)?,
        ));
        Ok(self)
    }

    /// Set the member, either the method name or signal name.
    pub fn member<'m: 'a, M>(mut self, member: M) -> Result<Self>
    where
        M: TryInto<MemberName<'m>>,
        M::Error: Into<Error>,
    {
        self.header
            .fields_mut()
            .replace(MessageField::Member(member.try_into().map_err(Into::into)?));
        Ok(self)
    }

    fn error_name<'e: 'a, E>(mut self, error: E) -> Result<Self>
    where
        E: TryInto<ErrorName<'e>>,
        E::Error: Into<Error>,
    {
        self.header.fields_mut().replace(MessageField::ErrorName(
            error.try_into().map_err(Into::into)?,
        ));
        Ok(self)
    }

    /// Set the name of the connection this message is intended for.
    pub fn destination<'d: 'a, D>(mut self, destination: D) -> Result<Self>
    where
        D: TryInto<BusName<'d>>,
        D::Error: Into<Error>,
    {
        self.header.fields_mut().replace(MessageField::Destination(
            destination.try_into().map_err(Into::into)?,
        ));
        Ok(self)
    }

    fn reply_to(mut self, reply_to: &MessageHeader<'_>) -> Result<Self> {
        let serial = reply_to.primary().serial_num().ok_or(Error::MissingField)?;
        self.header
            .fields_mut()
            .replace(MessageField::ReplySerial(*serial));

        if let Some(sender) = reply_to.sender()? {
            self.destination(sender.to_owned())
        } else {
            Ok(self)
        }
    }

    /// Build the [`Message`] with the given body.
    ///
    /// You may pass `()` as the body if the message has no body.
    ///
    /// The caller is currently required to ensure that the resulting message contains the headers
    /// as compliant with the [specification]. Additional checks may be added to this builder over
    /// time as needed.
    ///
    /// [specification]:
    /// https://dbus.freedesktop.org/doc/dbus-specification.html#message-protocol-header-fields
    pub fn build<B>(self, body: &B) -> Result<Message>
    where
        B: serde::ser::Serialize + DynamicType,
    {
        let ctxt = dbus_context!(0);

        // Note: this iterates the body twice, but we prefer efficient handling of large messages
        // to efficient handling of ones that are complex to serialize.
        #[cfg(unix)]
        let (body_len, fds_len) = zvariant::serialized_size_fds(ctxt, body)?;
        #[cfg(not(unix))]
        let body_len = zvariant::serialized_size(ctxt, body)?;

        let signature = body.dynamic_signature();

        self.build_generic(
            signature,
            body_len,
            move |cursor| {
                #[cfg(unix)]
                {
                    let (_, fds) = zvariant::to_writer_fds(cursor, ctxt, body)?;
                    Ok::<Vec<RawFd>, Error>(fds)
                }
                #[cfg(not(unix))]
                {
                    zvariant::to_writer(cursor, ctxt, body)?;
                    Ok::<(), Error>(())
                }
            },
            #[cfg(unix)]
            fds_len,
        )
    }

    /// Create a new message from a raw slice of bytes to populate the body with, rather than by
    /// serializing a value. The message body will be the exact bytes.
    ///
    /// # Safety
    ///
    /// This method is unsafe because it can be used to build an invalid message.
    pub unsafe fn build_raw_body<'b, S>(
        self,
        body_bytes: &[u8],
        signature: S,
        #[cfg(unix)] fds: Vec<RawFd>,
    ) -> Result<Message>
    where
        S: TryInto<Signature<'b>>,
        S::Error: Into<Error>,
    {
        let signature: Signature<'b> = signature.try_into().map_err(Into::into)?;
        #[cfg(unix)]
        let fds_len = fds.len();

        self.build_generic(
            signature,
            body_bytes.len(),
            move |cursor: &mut Cursor<&mut Vec<u8>>| {
                cursor.write_all(body_bytes)?;

                #[cfg(unix)]
                return Ok::<Vec<RawFd>, Error>(fds);

                #[cfg(not(unix))]
                return Ok::<(), Error>(());
            },
            #[cfg(unix)]
            fds_len,
        )
    }

    fn build_generic<WriteFunc>(
        self,
        mut signature: Signature<'_>,
        body_len: usize,
        write_body: WriteFunc,
        #[cfg(unix)] fds_len: usize,
    ) -> Result<Message>
    where
        WriteFunc: FnOnce(&mut Cursor<&mut Vec<u8>>) -> Result<BuildGenericResult>,
    {
        let ctxt = dbus_context!(0);
        let mut header = self.header;

        if !signature.is_empty() {
            if signature.starts_with(zvariant::STRUCT_SIG_START_STR) {
                // Remove leading and trailing STRUCT delimiters
                signature = signature.slice(1..signature.len() - 1);
            }
            header.fields_mut().add(MessageField::Signature(signature));
        }

        let body_len_u32 = body_len.try_into().map_err(|_| Error::ExcessData)?;
        header.primary_mut().set_body_len(body_len_u32);

        #[cfg(unix)]
        {
            let fds_len_u32 = fds_len.try_into().map_err(|_| Error::ExcessData)?;
            if fds_len != 0 {
                header.fields_mut().add(MessageField::UnixFDs(fds_len_u32));
            }
        }

        let hdr_len = zvariant::serialized_size(ctxt, &header)?;
        // We need to align the body to 8-byte boundary.
        let body_padding = padding_for_8_bytes(hdr_len);
        let body_offset = hdr_len + body_padding;
        let total_len = body_offset + body_len;
        if total_len > MAX_MESSAGE_SIZE {
            return Err(Error::ExcessData);
        }
        let mut bytes: Vec<u8> = Vec::with_capacity(total_len);
        let mut cursor = Cursor::new(&mut bytes);

        zvariant::to_writer(&mut cursor, ctxt, &header)?;
        for _ in 0..body_padding {
            cursor.write_all(&[0u8])?;
        }
        #[cfg(unix)]
        let fds = write_body(&mut cursor)?;
        #[cfg(not(unix))]
        write_body(&mut cursor)?;

        let primary_header = header.into_primary();
        let header: MessageHeader<'_> = zvariant::from_slice(&bytes, ctxt)?;
        let quick_fields = QuickMessageFields::new(&bytes, &header)?;

        Ok(Message {
            primary_header,
            quick_fields,
            bytes,
            body_offset,
            #[cfg(unix)]
            fds: Arc::new(RwLock::new(Fds::Raw(fds))),
            recv_seq: MessageSequence::default(),
        })
    }
}

impl<'m> From<MessageHeader<'m>> for MessageBuilder<'m> {
    fn from(mut header: MessageHeader<'m>) -> Self {
        // Signature and Fds are added by body* methods.
        let fields = header.fields_mut();
        fields.remove(MessageFieldCode::Signature);
        fields.remove(MessageFieldCode::UnixFDs);

        Self { header }
    }
}

#[cfg(test)]
mod tests {
    use super::MessageBuilder;
    use crate::Error;
    use test_log::test;

    #[test]
    fn test_raw() -> Result<(), Error> {
        let raw_body: &[u8] = &[16, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 3, 0, 0, 0, 4, 0, 0, 0];
        let message_builder = MessageBuilder::signal("/", "test.test", "test")?;
        let message = unsafe {
            message_builder.build_raw_body(
                raw_body,
                "ai",
                #[cfg(unix)]
                vec![],
            )?
        };

        let output: Vec<i32> = message.body()?;
        assert_eq!(output, vec![1, 2, 3, 4]);

        Ok(())
    }
}
