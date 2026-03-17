use anyhow::{Context, Result};
use std::path::Path;

/// Size of one CD+G subchannel packet in bytes.
const PACKET_SIZE: usize = 24;

/// The command byte value that identifies a valid CD+G packet (masked with 0x3F).
const CDG_COMMAND: u8 = 0x09;

/// A single CD+G subchannel packet (24 bytes from the CD subcode).
#[derive(Clone)]
pub struct CdgPacket {
    /// Must be 0x09 (masked) for a valid CDG command.
    pub command: u8,
    /// The instruction type (masked with 0x3F to get the actual instruction).
    pub instruction: u8,
    /// 16 bytes of instruction-specific data.
    pub data: [u8; 16],
}

impl CdgPacket {
    /// Whether this packet contains a valid CDG command.
    pub fn is_cdg(&self) -> bool {
        (self.command & 0x3F) == CDG_COMMAND
    }
}

/// Parse a `.cdg` file into a vector of packets.
///
/// Every 24 bytes in the file becomes one `CdgPacket`. Non-CDG packets are
/// included (they'll be skipped during rendering) to preserve timing — each
/// packet corresponds to 1/300th of a second.
pub fn parse_cdg_file(path: &Path) -> Result<Vec<CdgPacket>> {
    let bytes = std::fs::read(path)
        .with_context(|| format!("failed to read CDG file at {}", path.display()))?;

    let packet_count = bytes.len() / PACKET_SIZE;
    let mut packets = Vec::with_capacity(packet_count);

    for i in 0..packet_count {
        let offset = i * PACKET_SIZE;
        let mut data = [0u8; 16];
        data.copy_from_slice(&bytes[offset + 4..offset + 20]);

        packets.push(CdgPacket {
            command: bytes[offset],
            instruction: bytes[offset + 1],
            data,
        });
    }

    Ok(packets)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_empty_file() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("empty.cdg");
        std::fs::write(&path, b"").unwrap();
        let packets = parse_cdg_file(&path).unwrap();
        assert!(packets.is_empty());
    }

    #[test]
    fn parse_single_packet() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("test.cdg");
        let mut raw = [0u8; 24];
        raw[0] = 0x09; // valid CDG command
        raw[1] = 0x01; // MemoryPreset instruction
        raw[4] = 0x05; // color = 5 in data[0]
        std::fs::write(&path, raw).unwrap();

        let packets = parse_cdg_file(&path).unwrap();
        assert_eq!(packets.len(), 1);
        assert!(packets[0].is_cdg());
        assert_eq!(packets[0].instruction & 0x3F, 1);
        assert_eq!(packets[0].data[0] & 0x0F, 5);
    }

    #[test]
    fn non_cdg_packet_preserved_for_timing() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("test.cdg");
        let mut raw = [0u8; 48]; // 2 packets
        raw[0] = 0x00; // not a CDG command
        raw[24] = 0x09; // valid CDG
        raw[25] = 0x06; // TileBlock
        std::fs::write(&path, raw).unwrap();

        let packets = parse_cdg_file(&path).unwrap();
        assert_eq!(packets.len(), 2);
        assert!(!packets[0].is_cdg());
        assert!(packets[1].is_cdg());
    }

    #[test]
    fn trailing_bytes_ignored() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("test.cdg");
        // 24 bytes + 10 trailing bytes = only 1 packet
        let raw = [0u8; 34];
        std::fs::write(&path, raw).unwrap();

        let packets = parse_cdg_file(&path).unwrap();
        assert_eq!(packets.len(), 1);
    }
}
