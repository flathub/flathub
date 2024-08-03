//! Dbus abstraction
//! Documentation can be found at https://www.freedesktop.org/wiki/Software/systemd/dbus/

use std::collections::BTreeMap;
use std::collections::HashMap;

use log::debug;

/* use dbus::arg::messageitem::MessageItem;
use dbus::Message; */
use log::info;
use serde::Deserialize;
use zbus::blocking::fdo;
use zbus::blocking::Connection;
use zvariant::ObjectPath;
use zvariant::OwnedValue;
use zvariant::Type;

use crate::systemd::data::UnitInfo;
use crate::systemd::enums::ActiveState;
use crate::systemd::enums::UnitType;
use crate::widget::preferences::DbusLevel;

use super::enums::EnablementStatus;

use super::SystemdErrors;
use super::SystemdUnit;

const DESTINATION_SYSTEMD: &str = "org.freedesktop.systemd1";
//const INTERFACE_SYSTEMD_UNIT: &str = "org.freedesktop.systemd1.Unit";
const INTERFACE_SYSTEMD_MANAGER: &str = "org.freedesktop.systemd1.Manager";
const PATH_SYSTEMD: &str = "/org/freedesktop/systemd1";

const METHOD_LIST_UNIT: &str = "ListUnits";

const METHOD_LIST_UNIT_FILES: &str = "ListUnitFiles";

const METHOD_START_UNIT: &str = "StartUnit";
const METHOD_STOP_UNIT: &str = "StopUnit";
const METHOD_RESTART_UNIT: &str = "RestartUnit";
const METHOD_GET_UNIT_FILE_STATE: &str = "GetUnitFileState";

#[allow(dead_code)]
enum StartMode {
    ///If "replace" the call will start the unit and its dependencies,
    /// possibly replacing already queued jobs that conflict with this.
    Replace,

    ///If "fail" the call will start the unit and its dependencies, but will fail if this
    ///would change an already queued job.
    Fail,

    ///If "isolate" the call will start the unit in
    ///question and terminate all units that aren't dependencies of it.
    ///Note that "isolate" mode is invalid for method **StopUnit**.
    Isolate,

    ///If "ignore-dependencies" it will start a unit but ignore all its dependencies.
    IgnoreDependencies,

    ///If "ignore-requirements" it will start a unit but only ignore the requirement dependencies.
    IgnoreRequirements,
}

impl StartMode {
    fn as_str(&self) -> &'static str {
        match self {
            StartMode::Replace => "replace",
            StartMode::Fail => "fail",
            StartMode::Isolate => "isolate",
            StartMode::IgnoreDependencies => "ignore-dependencies",
            StartMode::IgnoreRequirements => "ignore-requirements",
        }
    }
}

/* /// Takes a systemd dbus function as input and returns the result as a `dbus::Message`.
fn dbus_message(function: &str) -> Result<Message, SystemdErrors> {
    let dest = DESTINATION_SYSTEMD;
    let path = PATH_SYSTEMD;
    let interface = INTERFACE_SYSTEMD_MANAGER;
    match dbus::Message::new_method_call(dest, path, interface, function) {
        Ok(message) => Ok(message),
        Err(error) => Err(SystemdErrors::DBusErrorStr(error)),
    }
}

/// Takes a `dbus::Message` as input and makes a connection to dbus, returning the reply.
fn dbus_connect(message: Message) -> Result<Message, SystemdErrors> {
    let connection = dbus::ffidisp::Connection::get_private(dbus::ffidisp::BusType::System)?;

    let message = connection.send_with_reply_and_block(message, 30000)?;

    Ok(message)
} */

/// Communicates with dbus to obtain a list of unit files and returns them as a `Vec<SystemdUnit>`.
pub fn list_unit_files(connection: &Connection) -> Result<Vec<SystemdUnit>, SystemdErrors> {
    let message = connection.call_method(
        Some(DESTINATION_SYSTEMD),
        PATH_SYSTEMD,
        Some(INTERFACE_SYSTEMD_MANAGER),
        METHOD_LIST_UNIT_FILES,
        &(),
    )?;

    let body = message.body();

    let array: Vec<LUnitFiles> = body.deserialize()?;

    let mut systemd_units: Vec<SystemdUnit> = Vec::with_capacity(array.len());

    for unit_file in array.iter() {
        let Some((_prefix, name_type)) = unit_file.primary_unit_name.rsplit_once('/') else {
            return Err(SystemdErrors::Malformed);
        };

        let Some((name, system_type)) = name_type.rsplit_once('.') else {
            return Err(SystemdErrors::Malformed);
        };

        let status_code = EnablementStatus::new(unit_file.enablement_status);
        let utype = UnitType::new(system_type);

        systemd_units.push(SystemdUnit {
            name: name.to_owned(),
            status_code,
            utype,
            path: unit_file.primary_unit_name.to_owned(),
        });
    }

    Ok(systemd_units)
}

#[derive(Deserialize, Type, PartialEq, Debug)]
struct LUnitFiles<'a> {
    primary_unit_name: &'a str,
    enablement_status: &'a str,
}

#[derive(Deserialize, Type, PartialEq, Debug)]
struct LUnit<'a> {
    primary_unit_name: &'a str,
    description: &'a str,
    load_state: &'a str,
    active_state: &'a str,
    sub_state: &'a str,
    followed_unit: &'a str,
    #[serde(borrow)]
    unit_object_path: ObjectPath<'a>,
    ///If there is a job queued for the job unit the numeric job id, 0 otherwise
    numeric_job_id: u32,
    job_type: &'a str,
    job_object_path: ObjectPath<'a>,
}

fn get_connection(level: DbusLevel) -> Result<Connection, SystemdErrors> {
    let connection = match level {
        DbusLevel::Session => zbus::blocking::Connection::session()?,
        DbusLevel::System => zbus::blocking::Connection::system()?,
    };

    Ok(connection)
}

fn list_units_description(
    connection: &Connection,
) -> Result<BTreeMap<String, UnitInfo>, SystemdErrors> {
    let message = connection.call_method(
        Some(DESTINATION_SYSTEMD),
        PATH_SYSTEMD,
        Some(INTERFACE_SYSTEMD_MANAGER),
        METHOD_LIST_UNIT,
        &(),
    )?;

    let body = message.body();

    let array: Vec<LUnit> = body.deserialize()?;

    let mut map: BTreeMap<String, UnitInfo> = BTreeMap::new();

    for service_struct in array.iter() {
        let active_state = ActiveState::from_str(service_struct.active_state);

        let unit = UnitInfo::new(
            service_struct.primary_unit_name,
            service_struct.description,
            service_struct.load_state,
            active_state,
            service_struct.sub_state,
            service_struct.followed_unit,
            service_struct.unit_object_path.as_str(),
        );

        map.insert(service_struct.primary_unit_name.to_ascii_lowercase(), unit);
    }

    Ok(map)
}

/// Returns the current enablement status of the unit
pub fn get_unit_file_state_path(level: DbusLevel, unit_file: &str) -> Result<EnablementStatus, SystemdErrors> {
    let connection = get_connection(level)?;

    let message = connection.call_method(
        Some(DESTINATION_SYSTEMD),
        PATH_SYSTEMD,
        Some(INTERFACE_SYSTEMD_MANAGER),
        METHOD_GET_UNIT_FILE_STATE,
        &(unit_file),
    )?;

    let body = message.body();
    let enablement_status: zvariant::Str = body.deserialize()?;

    Ok(EnablementStatus::new(enablement_status.as_str()))
}

pub fn list_units_description_and_state(
    level: DbusLevel,
) -> Result<BTreeMap<String, UnitInfo>, SystemdErrors> {
    let connection = get_connection(level)?;

    let mut units_map = list_units_description(&connection)?;

    let mut unit_files = list_unit_files(&connection)?;

    for unit_file in unit_files.drain(..) {
        match units_map.get_mut(&unit_file.full_name().to_ascii_lowercase()) {
            Some(unit_info) => {
                unit_info.set_file_path(unit_file.path);
                unit_info.set_enable_status(unit_file.status_code.to_string());
            }
            None => log::debug!(
                "Unit \"{}\" status \"{}\" not loaded!",
                unit_file.full_name(),
                unit_file.status_code.to_string()
            ),
        }
    }

    Ok(units_map)
}

/// Takes a unit name as input and attempts to start it
pub fn start_unit(level: DbusLevel, unit: &str) -> Result<String, SystemdErrors> {
    systemd_action(METHOD_START_UNIT, level, unit, StartMode::Fail)
}

/// Takes a unit name as input and attempts to stop it.
pub fn stop_unit(level: DbusLevel, unit: &str) -> Result<String, SystemdErrors> {
    systemd_action(METHOD_STOP_UNIT, level, unit, StartMode::Fail)
}

/// Enqeues a start job, and possibly depending jobs.
pub fn restart_unit(level: DbusLevel, unit: &str) -> Result<String, SystemdErrors> {
    systemd_action(METHOD_RESTART_UNIT, level, unit, StartMode::Fail)
}

fn systemd_action(
    method: &str,
    level: DbusLevel,
    unit: &str,
    mode: StartMode,
) -> Result<String, SystemdErrors> {
    let connection = get_connection(level)?;
    let message = connection.call_method(
        Some(DESTINATION_SYSTEMD),
        PATH_SYSTEMD,
        Some(INTERFACE_SYSTEMD_MANAGER),
        method,
        &(unit, mode.as_str()),
    )?;

    let body = message.body();
    let o: zvariant::ObjectPath = body.deserialize()?;

    let created_job_object = o.to_string();

    info!("{method} SUCCESS, response {created_job_object}");

    Ok(created_job_object)
}

fn convert_to_string(value: &zvariant::Value) -> String {
    let str_value: String = match value {
        zvariant::Value::U8(i) => i.to_string(),
        zvariant::Value::Bool(b) => b.to_string(),
        zvariant::Value::I16(i) => i.to_string(),
        zvariant::Value::U16(i) => i.to_string(),
        zvariant::Value::I32(i) => i.to_string(),
        zvariant::Value::U32(i) => i.to_string(),
        zvariant::Value::I64(i) => i.to_string(),
        zvariant::Value::U64(i) => i.to_string(),
        zvariant::Value::F64(i) => i.to_string(),
        zvariant::Value::Str(s) => s.to_string(),
        zvariant::Value::Signature(s) => s.to_string(),
        zvariant::Value::ObjectPath(op) => op.to_string(),
        zvariant::Value::Value(v) => v.to_string(),
        zvariant::Value::Array(a) => {
            let mut d_str = String::from("[ ");

            let mut it = a.iter().peekable();
            while let Some(mi) = it.next() {
                d_str.push_str(&convert_to_string(mi));
                if it.peek().is_some() {
                    d_str.push_str(", ");
                }
            }

            d_str.push_str(" ]");
            d_str
        }
        zvariant::Value::Dict(d) => {
            let mut d_str = String::from("{ ");
            for (mik, miv) in d.iter() {
                d_str.push_str(&convert_to_string(&mik));
                d_str.push_str(" : ");
                d_str.push_str(&convert_to_string(&miv));
            }
            d_str.push_str(" }");
            d_str
        }
        zvariant::Value::Structure(stc) => {
            let mut d_str = String::from("{ ");

            let mut it = stc.fields().iter().peekable();
            while let Some(mi) = it.next() {
                d_str.push_str(&convert_to_string(mi));
                if it.peek().is_some() {
                    d_str.push_str(", ");
                }
            }

            d_str.push_str(" }");
            d_str
        }
        zvariant::Value::Fd(fd) => fd.to_string(),
    };
    str_value
}

pub fn fetch_system_info(level: DbusLevel) -> Result<BTreeMap<String, String>, SystemdErrors> {
    fetch_system_unit_info(level, PATH_SYSTEMD)
}

pub fn fetch_system_unit_info(
    level: DbusLevel,
    path: &str,
) -> Result<BTreeMap<String, String>, SystemdErrors> {
    let connection = get_connection(level)?;

    let properties_proxy: zbus::blocking::fdo::PropertiesProxy =
        fdo::PropertiesProxy::builder(&connection)
            .destination(DESTINATION_SYSTEMD)?
            .path(path)?
            .build()?;

    let hm: HashMap<String, OwnedValue> = properties_proxy.get_all(None.into())?;

    let mut map = BTreeMap::new();

    for (key, value) in hm.iter() {
        debug!("{:?} {:?}", key, value);

        let str_val = convert_to_string(value);
        map.insert(key.to_owned(), str_val);
    }

    Ok(map)
}

#[cfg(test)]
mod tests {

    use std::collections::HashSet;

    /* use crate::systemd::collect_togglable_services; */

    use super::*;

    pub const TEST_SERVICE: &str = "jackett.service";

    fn init() {
        let _ = env_logger::builder()
            .target(env_logger::Target::Stdout)
            .filter_level(log::LevelFilter::Trace)
            .is_test(true)
            .try_init();
    }

    #[test]
    fn stop_service_test() -> Result<(), SystemdErrors> {
        stop_unit(DbusLevel::System, TEST_SERVICE)?;
        Ok(())
    }

/*     #[test]
    fn dbus_test() -> Result<(), SystemdErrors> {
        // let file: &str = "/etc/systemd/system/jackett.service";
        let file1: &str = TEST_SERVICE;
        let mut message = dbus_message("GetUnitFileState")?;

        let message_items = &[MessageItem::Str(file1.to_owned())];
        message.append_items(message_items);

        match dbus_connect(message) {
            Ok(m) => {
                debug!("{:?}", m.get1::<String>());
                Ok(())
            }
            Err(e) => Err(e),
        }
    } */

    #[test]
    fn test_get_unit_file_state() {
        // let file: &str = "/etc/systemd/system/jackett.service";
        let file1: &str = TEST_SERVICE;

        let status = get_unit_file_state_path(DbusLevel::System, file1);
        debug!("Status: {:?}", status);
    }

    #[test]
    fn test_list_unit_files() -> Result<(), SystemdErrors> {
        let units = list_unit_files(&get_connection(DbusLevel::System)?)?;

        let serv = units
            .iter()
            .filter(|ud| ud.full_name() == TEST_SERVICE)
            .nth(0);

        debug!("{:#?}", serv);
        Ok(())
    }

    #[test]
    fn test_list_units() -> Result<(), SystemdErrors> {
        let units = list_units_description(&get_connection(DbusLevel::System)?)?;

        let serv = units.get(TEST_SERVICE);
        debug!("{:#?}", serv);
        Ok(())
    }

    #[test]
    fn test_list_units_merge() -> Result<(), SystemdErrors> {
        let mut units_map = list_units_description(&get_connection(DbusLevel::System)?)?;

        let mut units = list_unit_files(&get_connection(DbusLevel::System)?)?;

        let mut set: HashSet<String> = HashSet::new();
        for unit_file in units.drain(..) {
            match units_map.get_mut(&unit_file.full_name().to_ascii_lowercase()) {
                Some(unit_info) => {
                    unit_info.set_file_path(unit_file.path);
                    unit_info.set_enable_status(unit_file.status_code.to_string());
                    unit_info.set_enable_status(unit_file.status_code.to_string());
                }
                None => debug!("unit \"{}\" not found!", unit_file.full_name()),
            }
        }

        debug!("{:#?}", units_map.get(TEST_SERVICE));

        for unit in units_map.values() {
            set.insert(unit.unit_type().to_owned());
        }

        debug!("Unit types {:#?}", set);

        Ok(())
    }

    /*  #[test]
        fn test_list_units_description_and_state() -> Result<(), SystemdErrors> {
           let units_map = list_units_description_and_state()?;

           let ts = units_map.get(TEST_SERVICE);
           debug!("Test Service {:#?}", ts);
           let units = units_map.into_values().collect::<Vec<LoadedUnit>>();

           let services = collect_togglable_services(&units);

           debug!("service.len {}", services.len());

           Ok(())
       }
    */
/* 
    #[test]
    fn test_prop() {
        init();
        let c = dbus::ffidisp::Connection::new_system().unwrap();
        let p = Props::new(
            &c,
            "org.freedesktop.PolicyKit1",
            "/org/freedesktop/PolicyKit1/Authority",
            "org.freedesktop.PolicyKit1.Authority",
            10000,
        );
        info!("BackendVersion: {:?}", p.get("BackendVersion").unwrap())
    } */

    /*     #[test]
    fn test_prop_all_systemd_manager() -> Result<(), SystemdErrors> {
        init();
        let c = dbus::ffidisp::Connection::new_system().unwrap();

        let dest = DESTINATION_SYSTEMD;
        let path = PATH_SYSTEMD;
        let interface = INTERFACE_SYSTEMD_MANAGER;
        let prop = Props::new(&c, dest, path, interface, 10000);

        let all_items = prop.get_all()?;
        log::info!("Systemd: {:#?}", all_items);

        for (a, b) in all_items.iter() {
            let str_val = display_message_item(b);
            log::info!("prop : {} \t value: {}", a, str_val);
        }

        Ok(())
    } */

/*     #[test]
    fn test_prop2() {
        init();
        let c = dbus::ffidisp::Connection::new_system().unwrap();

        let dest = DESTINATION_SYSTEMD;
        let path = "/org/freedesktop/systemd1";
        let interface = "org.freedesktop.systemd1.Manager";
        let prop = Props::new(&c, dest, path, interface, 10000);
        debug!("Version: {:?}", prop.get("Version").unwrap());
        debug!("Architecture: {:?}", prop.get("Architecture").unwrap());
    } */
/* 
    #[test]
    fn test_prop33() {
        init();
        let c = dbus::ffidisp::Connection::new_system().unwrap();

        let dest = "org.freedesktop.portal.Desktop";
        let path = "/org/freedesktop/portal/desktop";
        let interface = "org.freedesktop.portal.Settings.Read";
        let prop = Props::new(&c, dest, path, interface, 10000);

        match prop.get_all() {
            Ok(a) => println!("Results {:#?}", a),
            Err(e) => println!("Error! {:?}", e),
        }
        /*   debug!("Version: {:?}", prop.get("Version").unwrap());
        debug!("Architecture: {:?}", prop.get("Architecture").unwrap()); */
    } */

/*     #[test]
    fn test_prop34() -> Result<(), Box<dyn std::error::Error>> {
        let dest = "org.freedesktop.portal.Desktop";
        let path = "/org/freedesktop/portal/desktop";
        let interface = "org.freedesktop.portal.Settings.Read";
        let connection = dbus::blocking::Connection::new_session()?;
        let proxy = connection.with_proxy(dest, path, std::time::Duration::from_millis(5000));

        use super::dbus::blocking::stdintf::org_freedesktop_dbus::Properties;

        let metadata: super::dbus::arg::Variant<String> = proxy.get(interface, "Version")?;

        debug!("Meta: {:?}", metadata);
        Ok(())
    }

    #[test]
    fn test_prop3() -> Result<(), Box<dyn std::error::Error>> {
        let dest = DESTINATION_SYSTEMD;
        let path = "/org/freedesktop/systemd1";
        let interface = "org.freedesktop.systemd1.Manager";
        let connection = dbus::blocking::Connection::new_session()?;
        let proxy = connection.with_proxy(dest, path, std::time::Duration::from_millis(5000));

        use super::dbus::blocking::stdintf::org_freedesktop_dbus::Properties;

        let metadata: super::dbus::arg::Variant<String> = proxy.get(interface, "Version")?;

        debug!("Meta: {:?}", metadata);
        Ok(())
    }
 */
/*     #[test]
    pub fn test_get_unit_path() -> Result<(), SystemdErrors> {
        let unit_file: &str = TEST_SERVICE;
        let mut message = dbus_message("GetUnit")?;
        let message_items = &[MessageItem::Str(unit_file.to_owned())];
        message.append_items(message_items);

        let load_unit_ret = dbus_connect(message)?;
        debug!("{:?}", load_unit_ret);
        Ok(())
    } */

    /*     #[test]
    pub fn test_get_unit_parameters() {
        init();
        let dest = DESTINATION_SYSTEMD;
        let path = "/org/freedesktop/systemd1/unit/jackett_2eservice";

        let interface = INTERFACE_SYSTEMD_UNIT;
        let c = dbus::ffidisp::Connection::new_system().unwrap();
        let p = Props::new(&c, dest, path, interface, 10000);

        debug!("ALL PARAM: {:#?}", p.get_all());
    } */

    #[test]
    pub fn test_fetch_system_unit_info() -> Result<(), SystemdErrors> {
        init();

        let btree_map = fetch_system_unit_info(
            DbusLevel::System,
            "/org/freedesktop/systemd1/unit/jackett_2eservice",
        )?;

        debug!("ALL PARAM: {:#?}", btree_map);
        Ok(())
    }

/*     #[test]
    pub fn test_load_unit_() -> Result<(), SystemdErrors> {
        let unit_file: &str = TEST_SERVICE;
        let mut message = dbus_message("LoadUnit")?;
        let message_items = &[MessageItem::Str(unit_file.to_owned())];
        message.append_items(message_items);

        let load_unit_ret = dbus_connect(message)?;
        debug!("{:?}", load_unit_ret);
        Ok(())
    } */
}
