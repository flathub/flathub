/// Removable block devices
pub struct BlockDrive {
    pub block: String,
    pub drive: String,
    pub id: String,
}

impl BlockDrive {
    pub fn open(&self) -> Result<std::fs::File, Box<dyn std::error::Error>> {
        let mut arg = std::collections::HashMap::<&str, zvariant::Value>::new();
        arg.insert("flags", zvariant::Value::I32(libc::O_SYNC));
        let connection = zbus::Connection::new_system()?;
        let block_proxy = zbus::Proxy::new(
            &connection,
            "org.freedesktop.UDisks2",
            &self.block,
            "org.freedesktop.UDisks2.Block",
        )?;
        let fd: zvariant::Fd = block_proxy.call_method("OpenDevice", &("w", arg))?.body()?;
        use std::os::unix::io::{AsRawFd, FromRawFd};
        Ok(unsafe { std::fs::File::from_raw_fd(fd.as_raw_fd()) })
    }
}

pub fn get_disks() -> Result<Vec<BlockDrive>, Box<dyn std::error::Error>> {
    let mut result = Vec::<BlockDrive>::new();
    let connection = zbus::Connection::new_system()?;
    let udisks_proxy = zbus::Proxy::new(
        &connection,
        "org.freedesktop.UDisks2",
        "/org/freedesktop/UDisks2/Manager",
        "org.freedesktop.UDisks2.Manager",
    )?;
    let msg = udisks_proxy.call_method(
        "GetBlockDevices",
        &std::collections::HashMap::<String, zvariant::Value>::new(),
    )?;
    let resp: Vec<zvariant::ObjectPath> = msg.body()?;
    for op in resp {
        let block = op.as_str();
        // print!("{} ==> ", block);
        let block_proxy = zbus::Proxy::new(
            &connection,
            "org.freedesktop.UDisks2",
            block,
            "org.freedesktop.UDisks2.Block",
        )?;

        let block_msg = block_proxy.get_property::<zvariant::ObjectPath>("Drive")?;
        let drive = block_msg.as_str();

        // Eliminate /dev/sdxN && Eliminate "/"
        if block.as_bytes().last().unwrap() > &b'9' && drive.as_bytes().len() > 1 {
            let drive_proxy = zbus::Proxy::new(
                &connection,
                "org.freedesktop.UDisks2",
                drive,
                "org.freedesktop.UDisks2.Drive",
            )?;

            let ejectable_msg = drive_proxy.get_property::<bool>("Ejectable")?;
            let ejectable = ejectable_msg;

            if ejectable {
                let id_msg = drive_proxy.get_property::<String>("Id")?;
                let id = id_msg;
                let bd = BlockDrive {
                    block: String::from(block),
                    drive: String::from(drive),
                    id: id,
                };
                println!("Adding {} => {}, ({})", bd.block, bd.drive, bd.id);
                result.push(bd);
            }
        }
    }
    Ok(result)
}

use zbus::dbus_proxy;
use zvariant::Value;

#[dbus_proxy]
trait Notifications {
    fn notify(
        &self,
        app_name: &str,
        replaces_id: u32,
        app_icon: &str,
        summary: &str,
        body: &str,
        actions: &[&str],
        hints: std::collections::HashMap<&str, &Value>,
        expire_timeout: i32,
    ) -> zbus::Result<u32>;
}


pub fn end_notify() -> Result<(), Box<dyn std::error::Error>> {
    let connection = zbus::Connection::new_session()?;

    let proxy = NotificationsProxy::new(&connection)?;
    proxy.notify(
        "Nixwriter",
        0,
        "dialog-information",
        "Nixwriter has finished writing",
        "Your flash drive is now ready to boot",
        &[],
        std::collections::HashMap::new(),
        5000,
    )?;
    Ok(())
}