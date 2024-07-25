#![cfg_attr(
    all(not(debug_assertions), target_os = "windows"),
    windows_subsystem = "windows"
)]

use libhoptodesk::*;
#[cfg(feature = "standalone")]
use {
    std::{env, fs, ptr, ffi::CString, process::{Command, Stdio}, thread, time::Duration, os::windows::process::CommandExt},
    winapi::um::{libloaderapi::{GetModuleHandleA, GetModuleFileNameA, FreeLibrary}, winreg::{RegCreateKeyExA, RegSetValueExA, HKEY_CLASSES_ROOT, HKEY_CURRENT_USER}},
    crate::ui::get_dll_bytes,
    winapi::shared::minwindef::HKEY,
    winapi::um::winnt::REG_SZ
};
#[cfg(feature = "standalone")]
use std::fs::write;
#[cfg(not(any(target_os = "android", target_os = "ios")))]
use hbb_common::{config::{Config},};

#[cfg(windows)]
use nt_version;

#[cfg(any(target_os = "android", target_os = "ios", feature = "flutter"))]
fn main() {
    if !common::global_init() {
        return;
    }
    common::test_rendezvous_server();
    /*common::test_nat_type();
    #[cfg(target_os = "android")]
    crate::common::check_software_update();*/
    common::global_clean();
}

#[cfg(not(any(
    target_os = "android",
    target_os = "ios",
    feature = "cli",
    feature = "flutter"
)))]
fn main() {
	#[cfg(feature = "standalone")]
	if !crate::platform::is_installed() {
		let rule_name = "HopToDesk";
		let exe_path = env::current_exe().expect("Failed to get current executable path");
		let exe_path_new = env::current_exe().expect("Failed to retrieve current .exe path").to_string_lossy().into_owned();
		let root_key: HKEY = HKEY_CLASSES_ROOT as HKEY;
		let cu_key: HKEY = HKEY_CURRENT_USER as HKEY;

		let args: Vec<String> = env::args().collect();
		if args.len() > 1 && args[1] == "--fw" {
			let _ = crate::platform::windows::run_uac_hide("netsh",&format!("advfirewall firewall add rule name=\"{}\" dir=in action=allow program=\"{}\" enable=yes",rule_name,exe_path.display()));
			set_registry_string_value(&root_key, "hoptodesk", "URL Protocol", "").unwrap();
			set_registry_string_value(&root_key, "hoptodesk", "", "URL:hoptodesk Protocol").unwrap();
			set_registry_string_value(
				&root_key,
				"hoptodesk\\shell\\open\\command",
				"",
				&format!(r#""{}" "--connect" "%1""#, exe_path.to_str().expect("Failed to convert executable path to string")),
			).unwrap();	
			std::process::exit(0);
		}

		if args.len() > 1 && args[1] == "--ph" {
			let software_classes_key = format!("Software\\Classes\\{}", rule_name);
			set_registry_string_value(&cu_key, &software_classes_key, "URL Protocol", "").unwrap();
			set_registry_string_value(&cu_key, &software_classes_key, "", "URL:hoptodesk Protocol").unwrap();
			set_registry_string_value(
				&cu_key,
				&format!("{}\\shell\\open\\command", software_classes_key),
				"",
				&format!(r#""{}" "--connect" "%1""#, exe_path.to_str().expect("Failed to convert executable path to string")),
			).unwrap();
			
			std::process::exit(0);
		}

		let software_classes_key = format!("Software\\Classes\\{}", rule_name);
		set_registry_string_value(&cu_key, &software_classes_key, "URL Protocol", "").unwrap();
		set_registry_string_value(&cu_key, &software_classes_key, "", "URL:hoptodesk Protocol").unwrap();
		set_registry_string_value(
			&cu_key,
			&format!("{}\\shell\\open\\command", software_classes_key),
			"",
			&format!(r#""{}" "--connect" "%1""#, exe_path.to_str().expect("Failed to convert executable path to string")),
		).unwrap();

		
		let output = Command::new("netsh")
			.args(&[
				"advfirewall", "firewall", "show", "rule", "name=", rule_name, "verbose"
			])
			.stdout(Stdio::piped())
			.creation_flags(winapi::um::winbase::CREATE_NO_WINDOW)
			.output()
			.expect("Failed to execute netsh command.");

		let output_str = String::from_utf8_lossy(&output.stdout);
		if output_str.contains(rule_name) && output_str.contains(&exe_path_new) {
			//println!("Firewall rule already exists.");
		} else {
			let _ = crate::platform::windows::run_uac_hide(exe_path.to_str().expect("Failed to convert executable path to string"), "--fw");
			thread::sleep(Duration::from_secs(1));
		}
	
		let dll_bytes = get_dll_bytes();
		let dll_path = env::temp_dir().join("sciter.dll");
		
		let expected_size = if cfg!(target_arch = "x86") {
			6_036_992
		} else if cfg!(target_arch = "x86_64") {
			8_296_448
		} else {
			0 // Default size for other architectures, or handle differently
		};

		let file_size_matches = if let Ok(metadata) = fs::metadata(&dll_path) {
			metadata.len() == expected_size
		} else {
			false
		};

		if !file_size_matches {
			fs::write(&dll_path, dll_bytes).expect("Failed to write DLL file");
		}

	}
	

    #[cfg(feature = "standalone")]
    {
		let exe_path = env::current_exe().expect("Failed to get current executable file name");
		let exe_file_name = exe_path
			.file_name()
			.expect("Failed to extract file name")
			.to_string_lossy()
			.to_string();

			if let Some(id_start) = exe_file_name.find('-') {
				let id_part = &exe_file_name[id_start + 1..];
				let mut id_end = 0;
				for (i, c) in id_part.chars().enumerate() {
					if !(c.is_ascii_lowercase() || c.is_digit(10)) {
						break;
					}
					id_end = i + 1;
				}

				if id_end == 16 {
					let team_id = &id_part[..id_end];
		
		            let config_path = Config::path("TeamID.toml");
		            if let Some(parent_dir) = config_path.parent() {
		                if !parent_dir.exists() {
		                    fs::create_dir_all(parent_dir)
		                        .expect("Failed to create directory for TeamID.toml");
		                }
		            }
		            
					write(&Config::path("TeamID.toml"), team_id).expect("Failed to write team ID to file");
				}
			}
	}
	
	
    if !common::global_init() {
        return;
    }
    #[cfg(all(windows, not(feature = "inline")))]
    {
		let is_windows_7: bool;
		match nt_version::get() {
			(6, 1, _) => is_windows_7 = true,
			_ => is_windows_7 = false,
		}
	   if is_windows_7 {
			//println!("Windows 7 detected.");
		} else {
			let shellscalingapi = unsafe {
				match winapi::um::libloaderapi::LoadLibraryA("api-ms-win-shcore-scaling-l1-1-0.dll\0".as_ptr() as *const i8) {
					hmodule if !hmodule.is_null() => {
						let address = winapi::um::libloaderapi::GetProcAddress(hmodule, "SetProcessDpiAwareness\0".as_ptr() as *const i8);
						if !address.is_null() {
							Some(std::mem::transmute::<_, unsafe extern "system" fn(u32)>(address))
						} else {
							None
						}
					}
					_ => None,
				}
			};

			if let Some(set_process_dpi_awareness) = shellscalingapi {
				unsafe {
					set_process_dpi_awareness(2);
				}
			}
		
		}
    }
    if let Some(args) = crate::core_main::core_main().as_mut() {
        ui::start(args);
    }
	common::global_clean();
	#[cfg(feature = "standalone")]
	{
		if !crate::platform::is_installed() {
			let dll_name_cstring = CString::new("sciter.dll").expect("Failed to create CString");

		
			unsafe {
				let h_module = ptr::null_mut();
				let mut file_name = vec![0u8; 1024];
				let file_name_len = GetModuleFileNameA(h_module, file_name.as_mut_ptr() as *mut _, file_name.len() as u32);
				if file_name_len == 0 {
					panic!("Failed to get module file name");
				}
				let dll_name_cstring = dll_name_cstring.clone().into_bytes_with_nul();
				let dll_handle = GetModuleHandleA(dll_name_cstring.as_ptr() as *const _);
				if dll_handle.is_null() {
					panic!("Failed to get handle for DLL");
				}
				FreeLibrary(dll_handle);
				FreeLibrary(dll_handle);
			}
			let _ = std::fs::remove_file(env::temp_dir().join("sciter.dll")).ok();
		}
	}
}

#[cfg(feature = "cli")]
fn main() {
    if !common::global_init() {
        return;
    }
    use clap::App;
    use hbb_common::log;
    let args = format!(
        "-p, --port-forward=[PORT-FORWARD-OPTIONS] 'Format: remote-id:local-port:remote-port[:remote-host]'
        -c, --connect=[REMOTE_ID] 'test only'
        -k, --key=[KEY] ''
       -s, --server=[] 'Start server'",
    );
    let matches = App::new("hoptodesk")
        .version(crate::VERSION)
        .author("HopToDesk<info@hoptodesk.com>")
        .about("HopToDesk command line tool")
        .args_from_usage(&args)
        .get_matches();
    use hbb_common::{config::LocalConfig, env_logger::*};
    init_from_env(Env::default().filter_or(DEFAULT_FILTER_ENV, "info"));
    if let Some(p) = matches.value_of("port-forward") {
        let options: Vec<String> = p.split(":").map(|x| x.to_owned()).collect();
        if options.len() < 3 {
            log::error!("Wrong port-forward options");
            return;
        }
        let mut port = 0;
        if let Ok(v) = options[1].parse::<i32>() {
            port = v;
        } else {
            log::error!("Wrong local-port");
            return;
        }
        let mut remote_port = 0;
        if let Ok(v) = options[2].parse::<i32>() {
            remote_port = v;
        } else {
            log::error!("Wrong remote-port");
            return;
        }
        let mut remote_host = "localhost".to_owned();
        if options.len() > 3 {
            remote_host = options[3].clone();
        }
/*        
        common::test_rendezvous_server();
        common::test_nat_type();
        let key = matches.value_of("key").unwrap_or("").to_owned();
        let token = LocalConfig::get_option("access_token");
        cli::start_one_port_forward(
            options[0].clone(),
            port,
            remote_host,
            remote_port,
            key,
            token,
        );
    } else if let Some(p) = matches.value_of("connect") {
        common::test_rendezvous_server();
        common::test_nat_type();
        let key = matches.value_of("key").unwrap_or("").to_owned();
        let token = LocalConfig::get_option("access_token");
        cli::connect_test(p, key, token);
        */
    } else if let Some(p) = matches.value_of("server") {
        log::info!("id={}", hbb_common::config::Config::get_id());
        crate::start_server(true);
    }
    common::global_clean();
}

#[cfg(feature = "standalone")]
fn set_registry_string_value(root_key: &HKEY, key_path: &str, value_name: &str, value: &str) -> Result<(), String> {
    unsafe {
        let key_path = std::ffi::CString::new(key_path).unwrap();
        let value_name = std::ffi::CString::new(value_name).unwrap();
        let value = std::ffi::CString::new(value).unwrap();

        let mut hkey: HKEY = ptr::null_mut();
        let mut disposition = 0;
        if RegCreateKeyExA(*root_key,key_path.as_ptr(),0,ptr::null_mut(),0,winapi::um::winnt::KEY_WRITE,ptr::null_mut(),&mut hkey,&mut disposition,) != 0
        {
            return Err("Error creating or opening registry key".to_string());
        }

        if RegSetValueExA(
            hkey,
            value_name.as_ptr(),
            0,
            REG_SZ,
            value.as_ptr() as *const _,
            (value.as_bytes().len() + 1) as u32,
        ) != 0
        {
            return Err("Error setting registry value".to_string());
        }
    }

    Ok(())
}

