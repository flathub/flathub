pub mod analyze;
pub mod data;
mod sysdbus;
mod systemctl;

use std::collections::BTreeMap;
use std::env;
use std::process::{Command, Stdio};
use std::string::FromUtf8Error;

use data::UnitInfo;
use enums::{EnablementStatus, UnitType};
use gtk::glib::GString;
use log::{error, info, warn};
use std::fs::{self, File};
use std::io::{ErrorKind, Read, Write};

use crate::widget::preferences::DbusLevel;
use crate::widget::preferences::PREFERENCES;
use crate::widget::window;

pub mod enums;

const SYSDMNG_DIST_MODE: &str = "SYSDMNG_DIST_MODE";
const FLATPACK: &str = "flatpack";
const JOURNALCTL: &str = "journalctl";
const FLATPAK_SPAWN: &str = "flatpak-spawn";

#[derive(Debug)]
#[allow(unused)]
pub enum SystemdErrors {
    IoError(std::io::Error),
    Utf8Error(FromUtf8Error),
    SystemCtlError(String),
    DBusErrorStr(String),
    Malformed,
    ZBusError(zbus::Error),
    ZBusFdoError(zbus::fdo::Error),
}

impl From<std::io::Error> for SystemdErrors {
    fn from(error: std::io::Error) -> Self {
        SystemdErrors::IoError(error)
    }
}

impl From<FromUtf8Error> for SystemdErrors {
    fn from(error: FromUtf8Error) -> Self {
        SystemdErrors::Utf8Error(error)
    }
}

impl From<zbus::Error> for SystemdErrors {
    fn from(error: zbus::Error) -> Self {
        SystemdErrors::ZBusError(error)
    }
}

impl From<zbus::fdo::Error> for SystemdErrors {
    fn from(error: zbus::fdo::Error) -> Self {
        SystemdErrors::ZBusFdoError(error)
    }
}

#[derive(Clone, Debug)]
#[allow(unused)]
pub struct SystemdUnit {
    pub name: String,
    pub status_code: EnablementStatus,
    pub utype: UnitType,
    pub path: String,
}

impl SystemdUnit {
    pub fn full_name(&self) -> &str {
        match self.path.rsplit_once("/") {
            Some((_, end)) => end,
            None => &self.name,
        }
    }
}

pub fn get_unit_file_state(sytemd_unit: &UnitInfo) -> Result<EnablementStatus, SystemdErrors> {
    let level: DbusLevel = PREFERENCES.dbus_level().into();
    return sysdbus::get_unit_file_state_path(level, &sytemd_unit.primary());
}

pub fn list_units_description_and_state() -> Result<BTreeMap<String, UnitInfo>, SystemdErrors> {
    let level: DbusLevel = PREFERENCES.dbus_level().into();

    match sysdbus::list_units_description_and_state(level) {
        Ok(map) => Ok(map),
        Err(e) => {
            warn!("{:?}", e);
            Err(e)
        }
    }
}

/// Takes a unit name as input and attempts to start it
pub fn start_unit(unit: &UnitInfo) -> Result<String, SystemdErrors> {
    let level: DbusLevel = PREFERENCES.dbus_level().into();
    sysdbus::start_unit(level, &unit.primary())
}

/// Takes a unit name as input and attempts to stop it.
pub fn stop_unit(unit: &UnitInfo) -> Result<String, SystemdErrors> {
    let level: DbusLevel = PREFERENCES.dbus_level().into();
    sysdbus::stop_unit(level, &unit.primary())
}

pub fn restart_unit(unit: &UnitInfo) -> Result<String, SystemdErrors> {
    let level: DbusLevel = PREFERENCES.dbus_level().into();
    sysdbus::restart_unit(level, &unit.primary())
}

pub fn enable_unit_files(sytemd_unit: &UnitInfo) -> Result<EnablementStatus, SystemdErrors> {
    match systemctl::enable_unit_files_path(&sytemd_unit.primary()) {
        Ok(_) => Ok(EnablementStatus::Enabled),
        Err(e) => Err(e),
    }
}

pub fn disable_unit_files(sytemd_unit: &UnitInfo) -> Result<EnablementStatus, SystemdErrors> {
    match systemctl::disable_unit_files_path(&sytemd_unit.primary()) {
        Ok(_) => Ok(EnablementStatus::Disabled),
        Err(e) => Err(e),
    }
}

/// Read the unit file and return it's contents so that we can display it
pub fn get_unit_info(unit: &UnitInfo) -> String {
    let mut output = String::new();
    let Some(file_path) = &unit.file_path() else {
        return output;
    };

    if is_flatpak_mode() {
        match commander(&["cat", file_path]).output() {
            Ok(cat_output) => {
                match String::from_utf8(cat_output.stdout) {
                    Ok(content) => output.push_str(&content),
                    Err(e) => {
                        warn!("Can't retreive journal:  {:?}", e);
                        return output;
                    }
                };
            }
            Err(e) => {
                warn!("Can't open file \"{file_path}\" in cat, reason: {:?}", e);
                return output;
            }
        }
    } else {
        let mut file = match File::open(file_path) {
            Ok(f) => f,
            Err(e) => {
                warn!("Can't open file \"{file_path}\", reason: {:?}", e);
                return output;
            }
        };
        let _ = file.read_to_string(&mut output);
    }

    output
}

/// Obtains the journal log for the given unit.
pub fn get_unit_journal(unit: &UnitInfo) -> String {
    let unit_path = unit.primary();

    let outout = match commander(&[JOURNALCTL, "-b", "-u", &unit_path]).output() {
        Ok(output) => output.stdout,
        Err(e) => {
            warn!("Can't retreive journal:  {:?}", e);
            return String::new();
        }
    };

    let logs = match String::from_utf8(outout) {
        Ok(logs) => logs,
        Err(e) => {
            warn!("Can't retreive journal:  {:?}", e);
            return String::new();
        }
    };

    logs.lines()
        .rev()
        .map(|x| x.trim())
        .fold(String::with_capacity(logs.len()), |acc, x| acc + "\n" + x)
}

pub fn is_flatpak_mode() -> bool {
    match env::var(SYSDMNG_DIST_MODE) {
        Ok(val) => FLATPACK.eq(&val),
        Err(_) => false,
    }
}

pub fn commander(prog_n_args: &[&str]) -> Command {
    let output = if is_flatpak_mode() {
        let mut cmd = Command::new(FLATPAK_SPAWN);
        cmd.arg("--host");
        for v in prog_n_args {
            cmd.arg(v);
        }
        cmd
    } else {
        let mut cmd = Command::new(prog_n_args[0]);

        for i in 1..prog_n_args.len() {
            cmd.arg(prog_n_args[i]);
        }
        cmd
    };
    output
}

pub fn save_text_to_file(unit: &UnitInfo, text: &GString) {
    let Some(file_path) = &unit.file_path() else {
        error!("No file path for {}", unit.primary());
        return;
    };

    let mut file = match fs::OpenOptions::new().write(true).open(file_path) {
        Ok(file) => file,
        Err(err) => {
            match err.kind() {
                ErrorKind::PermissionDenied | ErrorKind::NotFound => {
                    write_with_priviledge(file_path, text);
                }
                _ => {
                    error!("Unable to open file: {:?}", err);
                }
            }
            return;
        }
    };

    match file.write(text.as_bytes()) {
        Ok(l) => error!("{l} bytes writen to {}", file_path),
        Err(err) => error!("Unable to write to file: {:?}", err),
    }
}

fn write_with_priviledge(file_path: &String, text: &GString) {
    let mut cmd = commander(&["pkexec", "tee", "tee", file_path]);
    let mut child = cmd
        .stdin(Stdio::piped())
        .stdout(Stdio::null())
        .spawn()
        .expect("failed to execute pkexec tee");
    /*     let mut child = std::process::Command::new("pkexec")
    .arg("tee")
    .arg(file_path)
    .stdin(Stdio::piped())
    .stdout(Stdio::null())
    .spawn()
    .expect("failed to execute pkexec tee"); */

    let child_stdin = match child.stdin.as_mut() {
        Some(cs) => cs,
        None => {
            error!("Unable to write to file: No stdin");
            return;
        }
    };

    match child_stdin.write_all(text.as_bytes()) {
        Ok(_) => info!("Write content as root on {}", file_path),
        Err(e) => error!("Write error: {:?}", e),
    }

    match child.wait() {
        Ok(exit) => info!("Subprocess exit code: {:?}", exit),
        Err(e) => error!("Failed to wait suprocess: {:?}", e),
    }
}

pub fn fetch_system_info() -> Result<BTreeMap<String, String>, SystemdErrors> {
    let level: DbusLevel = PREFERENCES.dbus_level().into();
    sysdbus::fetch_system_info(level)
}

pub fn fetch_system_unit_info(unit: &UnitInfo) -> Result<BTreeMap<String, String>, SystemdErrors> {
    let level: DbusLevel = PREFERENCES.dbus_level().into();
    sysdbus::fetch_system_unit_info(level, &unit.object_path())
}

pub fn test_flatpak_spawn(window: &window::Window) {
    if !is_flatpak_mode() {
        return;
    }

    match Command::new(FLATPAK_SPAWN).arg("--help").output() {
        Ok(_) => info!("flatpack-spawn check"),
        Err(_) => {
            let alert = gtk::AlertDialog::builder()
            .message("flatpack-spawn needed!")
            .detail("The program flatpack-spawn is needed if you use the application from Flatpack. Please install it to enable all features")
            .build();

            alert.show(Some(window));
        }
    }
}
