use gtk::glib;
use log::warn;
use log::info;
use std::fmt::Display;
use strum::EnumIter;

#[derive(Clone, Copy, Debug, PartialEq, Eq, EnumIter)]
pub enum EnablementStatus {
    Unknown = 0,
    Alias = 1,
    Bad = 2,
    Disabled = 3,
    Enabled = 4,
    Generated = 5,
    Indirect = 6,
    Linked = 7,
    Masked = 8,
    Static = 9,
    Trancient = 10,
}

impl EnablementStatus {
    /// Takes the string containing the state information from the dbus message and converts it
    /// into a UnitType by matching the first character.
    pub fn new(enablement_status: &str) -> EnablementStatus {
        if enablement_status.is_empty() {
            warn!("Empty Enablement Status: \"{}\"", enablement_status);
            return EnablementStatus::Unknown;
        }

        let c = enablement_status.chars().next().unwrap();

        match c {
            'a' => EnablementStatus::Alias,
            's' => EnablementStatus::Static,
            'd' => EnablementStatus::Disabled,
            'e' => EnablementStatus::Enabled,
            'i' => EnablementStatus::Indirect,
            'l' => EnablementStatus::Linked,
            'm' => EnablementStatus::Masked,
            'b' => EnablementStatus::Bad,
            'g' => EnablementStatus::Generated,
            't' => EnablementStatus::Trancient,
            _ => {
                warn!("Unknown State: {}", enablement_status);
                EnablementStatus::Unknown
            }
        }
    }

    pub fn to_str(&self) -> &str {
        let str_label = match self {
            EnablementStatus::Alias => "alias",
            EnablementStatus::Bad => "bad",
            EnablementStatus::Disabled => "disabled",
            EnablementStatus::Enabled => "enabled",
            EnablementStatus::Indirect => "indirect",
            EnablementStatus::Linked => "linked",
            EnablementStatus::Masked => "masked",
            EnablementStatus::Static => "static",
            EnablementStatus::Generated => "generated",
            EnablementStatus::Trancient => "trancient",
            EnablementStatus::Unknown => "",
        };

        str_label
    }

    pub fn to_string(&self) -> String {
        self.to_str().to_owned()
    }
}

impl From<Option<String>> for EnablementStatus {
    fn from(value: Option<String>) -> Self {
        if let Some(str_val) = value {
            return EnablementStatus::new(&str_val);
        }
        return EnablementStatus::Unknown;
    }
}

impl From<EnablementStatus> for u32 {
    fn from(value: EnablementStatus) -> Self {
        value as u32
    }
}

impl From<u32> for EnablementStatus {
    fn from(value: u32) -> Self {
        match value {
            0 => Self::Unknown,
            1 => Self::Alias,                         
            2 => Self::Bad,
            3 => Self::Disabled,
            4 => Self::Enabled,
            5 => Self::Generated,
            6 => Self::Indirect,
            7 => Self::Linked,
            8 => Self::Masked,
            9 => Self::Static,
            10 => Self::Trancient,
            _ => Self::Unknown,

        }
    }
}

#[derive(Clone, Copy, Default, Debug, PartialEq, Eq, glib::Enum, EnumIter)]
#[enum_type(name = "ActiveState")]
#[enum_dynamic]
pub enum ActiveState {
    Unknown = 0,
    Active = 1,
    #[default]
    Inactive = 2,
}

impl ActiveState {
    pub fn label(&self) -> &str {
        match self {
            ActiveState::Active => "active",
            ActiveState::Inactive => "inactive",
            ActiveState::Unknown => "unknown",
        }
    }

    pub fn icon_name(&self) -> &str {
        match self {
            ActiveState::Active => "object-select-symbolic",
            ActiveState::Inactive => "window-close-symbolic",
            ActiveState::Unknown => "action-unavailable-symbolic",
        }
    }

    pub fn from_str(input: &str) -> Self {
        match input {
            "active" => ActiveState::Active,
            "inactive" => ActiveState::Inactive,
            _ => ActiveState::Unknown,
        }
    }
}

impl Display for ActiveState {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.label())
    }
}

impl From<u32> for ActiveState {
    fn from(value: u32) -> Self {
        match value {
            0 => Self::Unknown,
            1 => Self::Active,
            2 => Self::Inactive,
            _ => Self::Unknown,
        }
    }
}

#[derive(Clone, Debug, PartialEq, Eq, EnumIter)]
pub enum UnitType {
    Automount,
    Busname,
    Device,
    Mount,
    Path,
    Scope,
    Service,
    Slice,
    Socket,
    Swap,
    Target,
    Timer,    
    Unknown(String),
}

impl UnitType {
    /// Takes the pathname of the unit as input to determine what type of unit it is.
    pub fn new(system_type: &str) -> UnitType {
        match system_type {
            "automount" => UnitType::Automount,
            "busname" => UnitType::Busname,
            "device" => UnitType::Device,
            "mount" => UnitType::Mount,
            "path" => UnitType::Path,
            "scope" => UnitType::Scope,
            "service" => UnitType::Service,
            "slice" => UnitType::Slice,
            "socket" => UnitType::Socket,
            "swap" => UnitType::Swap,
            "target" => UnitType::Target,
            "timer" => UnitType::Timer,          
            _ => {
                info!("Unknown Type: {}", system_type);
                UnitType::Unknown(system_type.to_string())
            }
        }
    }

    pub fn to_str(&self) -> &str {
        let str_label = match self {
            Self::Automount => "automount",
            Self::Busname => "busname",
            Self::Device => "device",
            Self::Mount => "mount",
            Self::Path => "path",
            Self::Scope => "scope",
            Self::Service => "service",
            Self::Slice => "slice",
            Self::Socket => "socket",
            Self::Target => "target",
            Self::Timer => "timer",
            Self::Swap => "swap",
            Self::Unknown(_) => "",
        };

        str_label
    }
}

#[cfg(test)]
mod tests {

    use super::*;

    #[test]
    fn test_enablement_status_any_number() {
        assert_eq!(
            <u32 as Into<EnablementStatus>>::into(1000),
            EnablementStatus::Unknown
        )
    }

    #[test]
    fn test_enablement_status_mapping() {
        //assert_num_mapping(EnablementStatus::Unasigned);
        assert_num_mapping(EnablementStatus::Bad);
        assert_num_mapping(EnablementStatus::Enabled);
        assert_num_mapping(EnablementStatus::Disabled);
        assert_num_mapping(EnablementStatus::Linked);
        assert_num_mapping(EnablementStatus::Masked);
        assert_num_mapping(EnablementStatus::Static);
        assert_num_mapping(EnablementStatus::Alias);
        assert_num_mapping(EnablementStatus::Generated);
        assert_num_mapping(EnablementStatus::Trancient);
        assert_num_mapping(EnablementStatus::Unknown);
    }

    fn assert_num_mapping(status: EnablementStatus) {
        let val = status as u32;
        let convert: EnablementStatus = val.into();
        assert_eq!(convert, status)
    }

    #[test]
    fn test_active_state_any_number() {
        assert_eq!(<u32 as Into<ActiveState>>::into(1000), ActiveState::Unknown)
    }

    #[test]
    fn test_active_state_mapping() {
        assert_num_mapping_active_state(ActiveState::Unknown);
        assert_num_mapping_active_state(ActiveState::Active);
        assert_num_mapping_active_state(ActiveState::Inactive);
    }

    fn assert_num_mapping_active_state(status: ActiveState) {
        let val = status as u32;
        let convert: ActiveState = val.into();
        assert_eq!(convert, status)
    }
}
