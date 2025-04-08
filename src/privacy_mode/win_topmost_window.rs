use super::{PrivacyMode, INVALID_PRIVACY_MODE_CONN_ID};
use crate::{privacy_mode::PrivacyModeState};
use hbb_common::{allow_err, bail, log, ResultType};
use std::ptr::null_mut;
use std::{
    ffi::CString,
    time::{Duration, Instant},
};
use winapi::{
    shared::{
        ntdef::{NULL},
        windef::HWND,
    },
    um::{
        handleapi::CloseHandle,
        processthreadsapi::{
            TerminateProcess,
        },
        winuser::*,
    },
};

#[cfg(feature = "standalone")]
use std::{env, fs, process::Command, path::PathBuf};

#[cfg(feature = "standalone")]
use crate::ui::get_dllpm_bytes;


pub(super) const PRIVACY_MODE_IMPL: &str = "privacy_mode_impl_mag";

pub const ORIGIN_PROCESS_EXE: &'static str = "C:\\Windows\\System32\\RuntimeBroker.exe";
pub const WIN_TOPMOST_INJECTED_PROCESS_EXE: &'static str = "RuntimeBroker_hoptodesk.exe";
pub const INJECTED_PROCESS_EXE: &'static str = WIN_TOPMOST_INJECTED_PROCESS_EXE;
pub(super) const PRIVACY_WINDOW_NAME: &'static str = "HopToDeskPrivacyWindow";

struct WindowHandlers {
    hthread: u64,
    hprocess: u64,
}

impl Drop for WindowHandlers {
    fn drop(&mut self) {
        self.reset();
    }
}

impl WindowHandlers {
    fn reset(&mut self) {
        unsafe {
            if self.hprocess != 0 {
                let _res = TerminateProcess(self.hprocess as _, 0);
                CloseHandle(self.hprocess as _);
            }
            self.hprocess = 0;
            if self.hthread != 0 {
                CloseHandle(self.hthread as _);
            }
            self.hthread = 0;
        }
    }

    fn is_default(&self) -> bool {
        self.hthread == 0 && self.hprocess == 0
    }
}

pub struct PrivacyModeImpl {
    impl_key: String,
    conn_id: i32,
    handlers: WindowHandlers,
    hwnd: u64,
}

impl PrivacyMode for PrivacyModeImpl {
    fn is_async_privacy_mode(&self) -> bool {
        false
    }
    
    fn init(&self) -> ResultType<()> {
        Ok(())
    }

    fn clear(&mut self) {
        allow_err!(self.turn_off_privacy(self.conn_id, None));
    }

    fn turn_on_privacy(&mut self, conn_id: i32) -> ResultType<bool> {
        if self.check_on_conn_id(conn_id)? {
            log::debug!("Privacy mode of conn {} is already on", conn_id);
            return Ok(true);
        }

        let exe_file = std::env::current_exe()?;
        if let Some(_cur_dir) = exe_file.parent() {
			#[cfg(not(feature = "standalone"))]
			if !_cur_dir.join("PrivacyMode.dll").exists() {
				return Ok(false);
			}
			#[cfg(feature = "standalone")]
			{
				let dll_bytes = get_dllpm_bytes();
				let dll_path = env::temp_dir().join("PrivacyMode.dll");
				if fs::metadata(&dll_path).is_err() {
					fs::write(&dll_path, dll_bytes).expect("Failed to write DLL file");
				}
		
				if !env::temp_dir().join("PrivacyMode.dll").exists() {
					return Ok(false);
				}
			}
        } else {
            bail!(
                "Invalid exe parent for {}",
                exe_file.to_string_lossy().as_ref()
            );
        }

        if self.handlers.is_default() {
            log::info!("turn_on_privacy, dll not found when started, try start");
            self.start()?;
            std::thread::sleep(std::time::Duration::from_millis(1_000));
        }

        let hwnd = wait_find_privacy_hwnd(0)?;
        if hwnd.is_null() {
            log::info!("No privacy window created");
        }
		log::info!("Privacy Window hwnd: {:?}", hwnd);
		unsafe {
			let ex_style = GetWindowLongPtrW(hwnd as HWND, GWL_EXSTYLE) as u32;
			let new_ex_style = ex_style | WS_EX_LAYERED | WS_EX_TRANSPARENT;
			SetWindowLongPtrW(hwnd as HWND, GWL_EXSTYLE, new_ex_style as i32);
			SetLayeredWindowAttributes(hwnd as HWND, 0, 255, LWA_ALPHA);
		}
			
        super::win_input::hook()?;
        unsafe {
            ShowWindow(hwnd as _, SW_SHOW);
        }
        self.conn_id = conn_id;
        self.hwnd = hwnd as _;
        Ok(true)
    }

    fn turn_off_privacy(
        &mut self,
        conn_id: i32,
        state: Option<PrivacyModeState>,
    ) -> ResultType<()> {
        self.check_off_conn_id(conn_id)?;
        super::win_input::unhook()?;

        unsafe {
            let hwnd = wait_find_privacy_hwnd(0)?;
            if !hwnd.is_null() {
                ShowWindow(hwnd, SW_HIDE);
            }
        }

        if self.conn_id != INVALID_PRIVACY_MODE_CONN_ID {
            if let Some(state) = state {
                allow_err!(super::set_privacy_mode_state(
                    conn_id,
                    state,
                    PRIVACY_MODE_IMPL.to_string(),
                    1_000
                ));
            }
            self.conn_id = INVALID_PRIVACY_MODE_CONN_ID.to_owned();
        }

        Ok(())
    }

    #[inline]
    fn pre_conn_id(&self) -> i32 {
        self.conn_id
    }

    #[inline]
    fn get_impl_key(&self) -> &str {
        &self.impl_key
    }
}

impl PrivacyModeImpl {
    pub fn new(impl_key: &str) -> Self {
        Self {
            impl_key: impl_key.to_owned(),
            conn_id: INVALID_PRIVACY_MODE_CONN_ID,
            handlers: WindowHandlers {
                hthread: 0,
                hprocess: 0,
            },
            hwnd: 0,
        }
    }

    #[inline]
    pub fn get_hwnd(&self) -> u64 {
        self.hwnd
    }

    pub fn start(&mut self) -> ResultType<()> {
        if self.handlers.hprocess != 0 {
            return Ok(());
        }

        log::info!("Start privacy mode window broker, check_update_broker_process");
        if let Err(e) = crate::platform::windows::check_update_broker_process() {
            log::warn!(
                "Failed to check update broker process. Privacy mode may not work properly. {}",
                e
            );
        }

		let program_name = "privacyhelper.exe";
		let exe_file = std::env::current_exe()?;

		let start_in_directory = if !crate::platform::is_installed() {
		    env::temp_dir().to_string_lossy().to_string()
		} else {
		    exe_file.parent().unwrap().to_string_lossy().to_string()
		};
		
		let mut program_path = PathBuf::from(&start_in_directory).join(program_name);
		
		if !program_path.exists() {
			program_path = PathBuf::from(&env::temp_dir().to_string_lossy().to_string()).join(program_name);
		} 
		
		if !program_path.exists() {
			program_path = PathBuf::from(&exe_file.parent().unwrap().to_string_lossy().to_string()).join(program_name);
		}
		
		log::info!("Program path: {:?}", program_path);

		if crate::platform::is_installed() {
			log::info!("Is installed: true");
		}
		
		if !program_path.exists() {
			log::info!("File does not exist: {:?}", program_path);
		}
	
		if !program_path.exists() {
			let exe_path = env::current_exe()?;
			program_path = exe_path.parent().map(|p| p.join(program_name)).unwrap_or(program_path);
			if !program_path.exists() {
				log::info!("Program not found at: {:?}", program_path);
				return Ok(());
			}
		}

		log::info!("Starting {:?} in {:?}", program_path, start_in_directory);
		
		let output = Command::new(&program_path)
			.current_dir(&start_in_directory)
			.spawn();

		match output {
			Ok(mut child) => {
				let exit_status = child.wait().expect("Failed to wait for child process");
				if exit_status.success() {
					log::info!("Privacy Helper ran successfully.");
				} else {
					log::info!("Privacy Helper failed with exit code: {:?}", exit_status.code());
				}
			}
			Err(e) => {
				log::info!("Error executing Privacy Helper: {:?}", e);
			}
		}
		
/*
		let class_name = CString::new("HopToDeskPrivacyWindow").expect("CString::new failed");
		let hwnd: HWND = unsafe { FindWindowA(class_name.as_ptr(), null_mut()) };

		if hwnd.is_null() {
			log::info!("Privacy Window not found.");
		} else {
			log::info!("Privacy Window handle: {:?}", hwnd);
		}
	*/

        Ok(())
    }

    #[inline]
    pub fn stop(&mut self) {
        self.handlers.reset();
    }
}

impl Drop for PrivacyModeImpl {
    fn drop(&mut self) {
        if self.conn_id != INVALID_PRIVACY_MODE_CONN_ID {
            allow_err!(self.turn_off_privacy(self.conn_id, None));
        }
    }
}
/*
unsafe fn inject_dll<'a>(hproc: HANDLE, hthread: HANDLE, dll_file: &'a str) -> ResultType<()> {
    let dll_file_utf16: Vec<u16> = dll_file.encode_utf16().chain(Some(0).into_iter()).collect();

    let buf = VirtualAllocEx(
        hproc,
        NULL as _,
        dll_file_utf16.len() * 2,
        MEM_COMMIT,
        PAGE_READWRITE,
    );
    if buf.is_null() {
        bail!("Failed VirtualAllocEx");
    }

    let mut written: usize = 0;
    if 0 == WriteProcessMemory(
        hproc,
        buf,
        dll_file_utf16.as_ptr() as _,
        dll_file_utf16.len() * 2,
        &mut written,
    ) {
        bail!("Failed WriteProcessMemory");
    }

    let kernel32_modulename = CString::new("kernel32")?;
    let hmodule = GetModuleHandleA(kernel32_modulename.as_ptr() as _);
    if hmodule.is_null() {
        bail!("Failed GetModuleHandleA");
    }

    let load_librarya_name = CString::new("LoadLibraryW")?;
    let load_librarya = GetProcAddress(hmodule, load_librarya_name.as_ptr() as _);
    if load_librarya.is_null() {
        bail!("Failed GetProcAddress of LoadLibraryW");
    }

    if 0 == QueueUserAPC(Some(std::mem::transmute(load_librarya)), hthread, buf as _) {
        bail!("Failed QueueUserAPC");
    }

    Ok(())
}
*/
pub(super) fn wait_find_privacy_hwnd(msecs: u128) -> ResultType<HWND> {
    let tm_begin = Instant::now();
    let wndname = CString::new(PRIVACY_WINDOW_NAME)?;
    loop {
        unsafe {
            let hwnd = FindWindowA(NULL as _, wndname.as_ptr() as _);
            if !hwnd.is_null() {
                return Ok(hwnd);
            }
        }

        if msecs == 0 || tm_begin.elapsed().as_millis() > msecs {
            return Ok(NULL as _);
        }

        std::thread::sleep(Duration::from_millis(100));
    }
}
