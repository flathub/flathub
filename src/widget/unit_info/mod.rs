use std::collections::HashMap;

use gtk::{prelude::*, Orientation};
use log::{error, info, warn};
use serde::Deserialize;
use time_handling::get_since_and_passed_time;
use zvariant::{DynamicType, OwnedValue, Type, Value};

use crate::systemd::{self, data::UnitInfo};

use super::info_window::InfoWindow;

mod time_handling;

pub fn fill_data(unit: &UnitInfo) -> gtk::Box {
    let info_box_main = gtk::Box::builder()
        .orientation(Orientation::Vertical)
        .spacing(5)
        .build();

    let info_box = gtk::Box::builder()
        .orientation(Orientation::Vertical)
        .spacing(5)
        .build();

    fill_all_info(&info_box, unit);

    info_box_main.append(&info_box);
    fill_buttons(&info_box_main, &info_box, unit);

    info_box_main
}

fn fill_all_info(info_box: &gtk::Box, unit: &UnitInfo) {
    while let Some(child) = info_box.last_child() {
        info_box.remove(&child)
    }

    fill_name_description(info_box, unit);

    let map = match systemd::fetch_system_unit_info_native(&unit) {
        Ok(m) => m,
        Err(e) => {
            error!("Fail to retreive Unit info: {:?}", e);
            HashMap::new()
        }
    };

    fill_description(info_box, &map);
    fill_dropin(info_box, &map);
    fill_active_state(info_box, &map);
    fill_load_state(info_box, &map);
    fill_docs(info_box, &map);
    fill_main_pid(info_box, &map, unit);
    fill_tasks(info_box, &map);
    fill_memory(info_box, &map);
    fill_cpu(info_box, &map);
    fill_trigger_timers_calendar(info_box, &map);
    fill_trigger_timers_monotonic(info_box, &map);
    fill_triggers(info_box, &map);
    fill_listen(info_box, &map);
    fill_control_group(info_box, &map);
}

fn fill_buttons(info_box_main: &gtk::Box, info_box: &gtk::Box, unit: &UnitInfo) {
    let refresh_button = gtk::Button::builder().label("Refresh").build();

    {
        let info_box = info_box.clone();
        let unit = unit.clone();
        refresh_button.connect_clicked(move |_a| {
            //systemd_gui::selected_unit(|unit: &UnitInfo| self.fill_data(unit));
            fill_all_info(&info_box, &unit);
        });
    }

    let show_all_button = gtk::Button::builder().label("Show All").build();

    {
        let unit2 = unit.clone();
        show_all_button.connect_clicked(move |_a| {
            let info_window = InfoWindow::new();

            info_window.fill_data(&unit2);

            info_window.present();
        });
    }

    let buttons_box = gtk::Box::builder()
        .orientation(Orientation::Horizontal)
        .spacing(5)
        .build();

    buttons_box.append(&refresh_button);
    buttons_box.append(&show_all_button);
    info_box_main.append(&buttons_box);
}

fn fill_name_description(info_box: &gtk::Box, unit: &UnitInfo) {
    fill_row(info_box, "Name:", &unit.primary());
    //fill_row(info_box, "Description:", &unit.description());
}

fn fill_row(info_box: &gtk::Box, key_label: &str, value: &str) {
    let item = gtk::Box::builder()
        .orientation(Orientation::Horizontal)
        .spacing(5)
        .width_request(30)
        .build();

    let key_label = gtk::Label::builder()
        .label(key_label)
        .width_request(130)
        .build();

    item.append(&key_label);

    let label_value = gtk::Label::builder().label(value).selectable(true).build();

    item.append(&label_value);

    info_box.append(&item);
}

macro_rules! get_value {
    ($map:expr, $key:expr) => {
        get_value!($map, $key, ())
    };

    ($map:expr, $key:expr, $dft:expr) => {{
        let Some(value) = $map.get($key) else {
            info!("Key doesn't exists: {:?}", $key);
            return $dft;
        };
        value
    }};
}

fn fill_dropin(info_box: &gtk::Box, map: &HashMap<String, OwnedValue>) {
    let value = get_value!(map, "DropInPaths");

    let drop_in_paths = get_array_str(value);

    if drop_in_paths.is_empty() {
        return;
    }

    let mut drop_in = String::new();
    for s in drop_in_paths {
        let (first, last) = s.rsplit_once('/').unwrap();
        drop_in.push_str(first);
        drop_in.push('\n');
        drop_in.push_str("└─");
        drop_in.push_str(last);
        drop_in.push('\n');
    }

    fill_row(info_box, "Drop in:", &drop_in);
}

fn fill_active_state(info_box: &gtk::Box, map: &HashMap<String, OwnedValue>) {
    let value = get_value!(map, "ActiveState");
    let state = value_str(value);

    let mut state_line = String::from(state);
    
    if let Some(substate) = get_substate(map) {
        state_line.push_str(" (");
        state_line.push_str(substate);
        state_line.push(')');
    }
    
    if let Some(since) = add_since(map, state)  {
        state_line.push_str(" since ");
        state_line.push_str(&since.0);
        state_line.push_str("; ");
        state_line.push_str(&since.1);
        state_line.push_str(" ago");
    }
 
    fill_row(info_box, "Active State:", &state_line)
}

fn get_substate(map: &HashMap<String, OwnedValue>) -> Option<&str> {
    let value = get_value!(map, "SubState", None);
    Some(value_str(value))
}

fn add_since(map: &HashMap<String, OwnedValue>, state: &str) -> Option<(String, String)> {
    let key = match state {
        "active" => "ActiveEnterTimestamp",
        "inactive" => "InactiveEnterTimestamp",
        _ => "StateChangeTimestamp",
    };

    let value = get_value!(map, key, None);

    let duration = value_u64(value);

    let since = get_since_and_passed_time(duration);

    Some(since)
}

fn fill_description(info_box: &gtk::Box, map: &HashMap<String, OwnedValue>) {
    let value = get_value!(map, "Description");
    fill_row(info_box, "Description:", value_str(value));
}

fn fill_load_state(info_box: &gtk::Box, map: &HashMap<String, OwnedValue>) {
    let value = get_value!(map, "LoadState");
    fill_row(info_box, "Load State:", value_str(value));
}

fn fill_docs(info_box: &gtk::Box, map: &HashMap<String, OwnedValue>) {
    let value = get_value!(map, "Documentation");

    let docs = get_array_str(value);

    if docs.is_empty() {
        return;
    }

    fill_row(info_box, "Doc:", &docs.join("\n"));
}

fn get_array_str<'a>(value: &'a zvariant::Value<'a>) -> Vec<&'a str> {
    let vec = match value as &zvariant::Value {
        zvariant::Value::Array(a) => {
            let mut vec = Vec::with_capacity(a.len());

            let mut it = a.iter();
            while let Some(mi) = it.next() {
                vec.push(value_str(mi));
            }

            vec
        }
        _ => {
            warn!("Wrong zvalue conversion: {:?}", value.dynamic_signature());
            return Vec::new();
        }
    };
    vec
}

fn fill_memory(info_box: &gtk::Box, map: &HashMap<String, OwnedValue>) {
    let value = get_value!(map, "MemoryCurrent");

    let memory_current = value_u64(value);
    if memory_current == U64MAX {
        return;
    }

    let value_str = &human_bytes(memory_current);
    fill_row(info_box, "Memory:", value_str);
}

fn fill_main_pid(info_box: &gtk::Box, map: &HashMap<String, OwnedValue>, unit: &UnitInfo) {
    let main_pid = get_main_pid(map);

    if 0 == main_pid {
    } else {
        let exec_val = if let Some(exec) = get_exec(map) {
            exec
        } else {
            &unit.display_name()
        };

        let v = &format!("{} ({})", main_pid, exec_val);
        fill_row(info_box, "Main PID:", v);
    }
}

fn get_main_pid(map: &HashMap<String, OwnedValue>) -> u32 {
    let value = get_value!(map, "MainPID", 0);

    if let zvariant::Value::U32(main_pid) = value as &Value {
        return *main_pid;
    }
    0
}

fn get_exec_full<'a>(map: &'a HashMap<String, OwnedValue>) -> Option<&'a str> {
    let value = get_value!(map, "ExecStart", None);

    if let zvariant::Value::Array(array) = value as &Value {
        if let Ok(Some(owned_value)) = array.get::<&Value>(0) {
            if let zvariant::Value::Structure(zstruc) = owned_value {
                if let Some(val_0) = zstruc.fields().get(0) {
                    if let zvariant::Value::Str(zstr) = val_0 {
                        return Some(zstr);
                    }
                }
            }
        }
    }

    None
}

fn get_exec<'a>(map: &'a HashMap<String, OwnedValue>) -> Option<&'a str> {
    if let Some(exec_full) = get_exec_full(map) {
        if let Some((_pre, last)) = exec_full.rsplit_once('/') {
            return Some(last);
        }
    }
    None
}

fn fill_cpu(info_box: &gtk::Box, map: &HashMap<String, OwnedValue>) {
    let value = get_value!(map, "CPUUsageNSec");

    let value_u64 = value_u64(value);
    if value_u64 == U64MAX {
        return;
    }

    let value_str = &human_time(value_u64);
    fill_row(info_box, "CPU:", value_str);
}

fn fill_tasks(info_box: &gtk::Box, map: &HashMap<String, OwnedValue>) {
    let value = get_value!(map, "TasksCurrent");

    let value_nb = value_u64(value);

    if value_nb == U64MAX {
        return;
    }

    let mut tasks_info = value_nb.to_string();

    if let Some(value) = map.get("TasksMax") {
        tasks_info.push_str(" (limit: ");
        let value_u64 = value_u64(value);
        tasks_info.push_str(&value_u64.to_string());
        tasks_info.push_str(")");
    }

    fill_row(info_box, "Tasks:", &tasks_info);
}

fn fill_trigger_timers_calendar(info_box: &gtk::Box, map: &HashMap<String, OwnedValue>) {
    let value = get_value!(map, "TimersCalendar");

    let zvariant::Value::Array(array) = value as &Value else {
        return;
    };

    if array.is_empty() {
        return;
    }

    let Ok(Some(val_listen_stc)) = array.get::<&Value>(0) else {
        return;
    };

    let zvariant::Value::Structure(zstruc) = val_listen_stc else {
        return;
    };

    let Some(zvariant::Value::Str(val_0)) = zstruc.fields().get(0) else {
        return;
    };

    let Some(zvariant::Value::Str(val_1)) = zstruc.fields().get(1) else {
        return;
    };

    let Some(zvariant::Value::U64(_val_2)) = zstruc.fields().get(2) else {
        return;
    };

    let timers = format!("{} {}", val_0, val_1);

    fill_row(info_box, "Trigger:", &timers);
}

fn fill_trigger_timers_monotonic(info_box: &gtk::Box, map: &HashMap<String, OwnedValue>) {
    let value = get_value!(map, "TimersMonotonic");

    let zvariant::Value::Array(array) = value as &Value else {
        return;
    };

    if array.is_empty() {
        return;
    }

    let timers = value.to_string();

    if timers.is_empty() {
        return;
    }

    fill_row(info_box, "Trigger:", &timers);
}

fn fill_triggers(info_box: &gtk::Box, map: &HashMap<String, OwnedValue>) {
    let value = get_value!(map, "Triggers");

    let triggers = get_array_str(value);

    if triggers.is_empty() {
        return;
    }

    //TODO add the active state of the triggers

    fill_row(info_box, "Triggers:", &triggers.join("\n"));
}

#[derive(Deserialize, Type, PartialEq, Debug)]
struct Struct {
    field1: String,
    field2: String,
}

fn fill_listen(info_box: &gtk::Box, map: &HashMap<String, OwnedValue>) {
    let value = get_value!(map, "Listen");

    let zvariant::Value::Array(array) = value as &Value else {
        return;
    };

    let Ok(Some(val_listen_stc)) = array.get::<&Value>(0) else {
        return;
    };

    let zvariant::Value::Structure(zstruc) = val_listen_stc else {
        return;
    };

    let Some(zvariant::Value::Str(val_0)) = zstruc.fields().get(0) else {
        return;
    };

    let Some(zvariant::Value::Str(val_1)) = zstruc.fields().get(1) else {
        return;
    };

    let listen = format!("{} ({})", val_1, val_0);

    fill_row(info_box, "Listen:", &listen);
}

fn fill_control_group(info_box: &gtk::Box, map: &HashMap<String, OwnedValue>) {
    let value = get_value!(map, "ControlGroup");

    let c_group = value_str(value);

    if c_group.is_empty() {
        return;
    }

    const KEY_LABEL: &str = "CGroup:";

    if let Some(exec_full) = get_exec_full(map) {
        let main_pid = get_main_pid(map);

        let mut group = String::new();

        group.push_str(c_group);
        group.push('\n');
        group.push_str("└─");
        group.push_str(&main_pid.to_string());
        group.push(' ');
        group.push_str(exec_full);
        group.push('\n');

        fill_row(info_box, KEY_LABEL, &group);
    } else {
        fill_row(info_box, KEY_LABEL, c_group);
    }
}

fn value_str<'a>(value: &'a Value<'a>) -> &'a str {
    if let zvariant::Value::Str(converted) = value as &Value {
        return converted.as_str();
    }
    warn!("Wrong zvalue conversion: {:?}", value);
    ""
}

/// 2^16-1
const U64MAX: u64 = 18_446_744_073_709_551_615;
const SUFFIX: [&str; 9] = ["B", "K", "M", "G", "T", "P", "E", "Z", "Y"];
const UNIT: f64 = 1024.0;

fn value_u64(value: &Value) -> u64 {
    if let zvariant::Value::U64(converted) = value {
        return *converted;
    }
    warn!("Wrong zvalue conversion: {:?}", value);
    U64MAX
}

/// Converts bytes to human-readable values
fn human_bytes(bytes: u64) -> String {
    // let size: f64 = *bytes as f64;

    if bytes <= 0 {
        return "0 B".to_string();
    }

    let base = (bytes as f64).log10() / UNIT.log10();

    let mut result: String = format!("{:.1}", UNIT.powf(base - base.floor()))
        .trim_end_matches(".0")
        .to_string();

    result.push_str(" ");
    result.push_str(SUFFIX[base.floor() as usize]);

    result
}

const T_SUFFIX: [&str; 9] = ["ns", "us", "ms", "s", "Ks", "Ms", "Gs", "Ts", "Ps"];
const T_UNIT: f64 = 1000.0;

fn human_time(value: u64) -> String {
    if value <= 0 {
        return "0".to_string();
    }

    let base = (value as f64).log10() / T_UNIT.log10();
    let v = T_UNIT.powf(base - base.floor());

    let mut result: String = if value <= 1_000_000_000 {
        format!("{:.0}", v)
    } else {
        format!("{:.3}", v)
    }
    .trim_end_matches(".0")
    .to_string();

    result.push_str(" ");
    result.push_str(T_SUFFIX[base.floor() as usize]);

    result
}

#[cfg(test)]
mod tests {

    use super::*;
    #[test]
    fn test1() {
        println!("{}", human_bytes(0));
        println!("{}", human_bytes(3));
        println!("{}", human_bytes(18446744073709551615));
        println!("{}", human_bytes(1024));
    }

    #[test]
    fn test2() {
        println!("{}", human_time(0));
        println!("{}", human_time(3));
        //println!("{}", human_time(U64MAX));
        println!("{}", human_time(1024));
        println!("{}", human_time(1_606_848_000));
        println!("{}", human_time(3_235_000));
        println!("{}", human_time(32_235_000));
        println!("{}", human_time(321_235_000));
        println!("{}", human_time(3_234_235_000));
    }
}
