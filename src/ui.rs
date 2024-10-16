use copypasta::{ClipboardContext, ClipboardProvider};
use std::{
    collections::HashMap,
    iter::FromIterator,
    process::Child,
    sync::{Arc, Mutex},
};
//use tokio::time::{Duration};
use sciter::Value;
use hbb_common::rand;
//use std::fs::write;
use hbb_common::{
    allow_err,
    config::{Config, LocalConfig, PeerConfig},
    log,
    //rendezvous_proto::*,
    tokio::{self},
};

#[cfg(not(any(feature = "flutter", feature = "cli")))]
use crate::ui_session_interface::Session;
use crate::{common::get_app_name, ipc, ui_interface::*};
use hbb_common::get_version_number;

mod cm;
#[cfg(feature = "inline")]
pub mod inline;
pub mod remote;

pub type Children = Arc<Mutex<(bool, HashMap<(String, String), Child>)>>;
#[allow(dead_code)]
type Status = (i32, bool, i64, String);

lazy_static::lazy_static! {
    // stupid workaround for https://sciter.com/forums/topic/crash-on-latest-tis-mac-sdk-sometimes/
    static ref STUPID_VALUES: Mutex<Vec<Arc<Vec<Value>>>> = Default::default();
}

#[cfg(not(any(feature = "flutter", feature = "cli")))]
lazy_static::lazy_static! {
    pub static ref CUR_SESSION: Arc<Mutex<Option<Session<remote::SciterHandler>>>> = Default::default();
    static ref CHILDREN : Children = Default::default();
}

struct UIHostHandler;

	
use std::env;

#[cfg(feature = "standalone")]
static DLL_BYTES: &[u8] = include_bytes!("../../sciter.dll");
#[cfg(feature = "standalone")]
static DLL_BYTESPM: &[u8] = include_bytes!("../../PrivacyMode.dll");
#[cfg(feature = "standalone")]
static DLL_BYTESPH: &[u8] = include_bytes!("../../privacyhelper.exe");


pub fn start(args: &mut [String]) {
    #[cfg(all(feature = "standalone", target_os = "windows"))]
	if !crate::platform::is_installed() {
		let dll_path = env::temp_dir().join("sciter.dll");
		let dll_path_str = dll_path.to_str().expect("Failed to convert path to string");
		sciter::set_library(dll_path_str).ok();
	} else {
		use std::path::Path;
		use std::fs;
		if !Path::new("sciter.dll").exists() {
			let dll_bytes = get_dll_bytes();
			let dll_path = env::temp_dir().join("sciter.dll");
			let dll_path_str = dll_path.to_str().expect("Failed to convert path to string");			
			if fs::metadata(&dll_path).is_err() {
				fs::write(&dll_path, dll_bytes).expect("Failed to write DLL file");
				sciter::set_library(dll_path_str).ok();
			}
			sciter::set_library(dll_path_str).ok();			
		}			
	}
	#[cfg(target_os = "macos")]
    crate::platform::delegate::show_dock();
    #[cfg(all(target_os = "linux", feature = "inline"))]
    {
        let app_dir = std::env::var("APPDIR").unwrap_or("".to_string());
        let mut so_path = "/usr/lib/rustdesk/libsciter-gtk.so".to_owned();
        for (prefix, dir) in [
            ("", "/usr"),
            ("", "/app"),
            (&app_dir, "/usr"),
            (&app_dir, "/app"),
        ]
        .iter()
        {
            let path = format!("{prefix}{dir}/lib/rustdesk/libsciter-gtk.so");
            if std::path::Path::new(&path).exists() {
                so_path = path;
                break;
            }
        }
        sciter::set_library(&so_path).ok();
    }
    #[cfg(windows)]
    // Check if there is a sciter.dll nearby.
    if let Ok(exe) = std::env::current_exe() {
        if let Some(parent) = exe.parent() {
            let sciter_dll_path = parent.join("sciter.dll");
            if sciter_dll_path.exists() {
                // Try to set the sciter dll.
                let p = sciter_dll_path.to_string_lossy().to_string();
                log::debug!("Found dll:{}, \n {:?}", p, sciter::set_library(&p));
            }
        }
    }    
    // https://github.com/c-smile/sciter-sdk/blob/master/include/sciter-x-types.h
    // https://github.com/rustdesk/rustdesk/issues/132#issuecomment-886069737
    #[cfg(windows)]
    allow_err!(sciter::set_options(sciter::RuntimeOptions::GfxLayer(
        sciter::GFX_LAYER::WARP
    )));
    use sciter::SCRIPT_RUNTIME_FEATURES::*;
    allow_err!(sciter::set_options(sciter::RuntimeOptions::ScriptFeatures(
        ALLOW_FILE_IO as u8 | ALLOW_SOCKET_IO as u8 | ALLOW_EVAL as u8 | ALLOW_SYSINFO as u8
    )));
    let mut frame = sciter::WindowBuilder::main_window().create();
    #[cfg(feature = "packui")]
    {
        let resources = include_bytes!("../target/resources.rc");
        frame.archive_handler(resources).expect("Invalid archive");
    }
    #[cfg(windows)]
    allow_err!(sciter::set_options(sciter::RuntimeOptions::UxTheming(true)));
    frame.set_title(&crate::get_app_name());
    #[cfg(target_os = "macos")]
    crate::platform::delegate::make_menubar(frame.get_host(), args.is_empty());
    #[cfg(windows)]
    crate::platform::try_set_window_foreground(frame.get_hwnd() as _);
    let page;
    if args.len() > 1 && args[0] == "--play" {
        args[0] = "--connect".to_owned();
        let path: std::path::PathBuf = (&args[1]).into();
        let id = path
            .file_stem()
            .map(|p| p.to_str().unwrap_or(""))
            .unwrap_or("")
            .to_owned();
        args[1] = id;
    }
    let args_string = args.concat().replace("\"", "").replace("[", "").replace("]", "");
	
	if args.is_empty() || args_string.is_empty() {
		//let children: Children = Default::default();
        //std::thread::spawn(move || check_zombie(children));
        std::thread::spawn(move || check_zombie());
        set_version();
        frame.event_handler(UI {});
        frame.sciter_handler(UIHostHandler {});
        page = "index.html";
        // Start pulse audio local server.
        #[cfg(target_os = "linux")]
        std::thread::spawn(crate::ipc::start_pa);
    } else if args[0] == "--install" {
        frame.event_handler(UI {});
        frame.sciter_handler(UIHostHandler {});
        page = "install.html";
    } else if args[0] == "--cm" {
        frame.register_behavior("connection-manager", move || {
            Box::new(cm::SciterConnectionManager::new())
        });
        page = "cm.html";
    } else if (args[0] == "--connect"
        || args[0] == "--file-transfer"
        || args[0] == "--port-forward"
        || args[0] == "--rdp")
        && args.len() > 1
    {
        #[cfg(windows)]
        {
            let hw = frame.get_host().get_hwnd();
            crate::platform::windows::enable_lowlevel_keyboard(hw as _);
        }
        let mut iter = args.iter();
        let cmd = iter.next().unwrap().clone();
        let id = iter.next().unwrap().clone();
        let pass = iter.next().unwrap_or(&"".to_owned()).clone();
		let _teamid = iter.next().unwrap_or(&"".to_owned()).clone();
		let tokenexp = iter.next().unwrap_or(&"".to_owned()).clone();
        let args: Vec<String> = iter.map(|x| x.clone()).collect();
		if id == "hoptodesk:///" || id.is_empty()  {
			return;
		}
		if !tokenexp.is_empty() {
			std::fs::write(&Config::path("LastToken.toml"), tokenexp.clone()).expect("Failed to write tokenexp to file");
		}
        frame.set_title(&id);
        frame.register_behavior("native-remote", move || {
            let handler =
                remote::SciterSession::new(cmd.clone(), id.clone(), pass.clone(), tokenexp.clone(), args.clone());
            #[cfg(not(any(feature = "flutter", feature = "cli")))]
            {
                *CUR_SESSION.lock().unwrap() = Some(handler.inner());
            }
            Box::new(handler)
        });
        page = "remote.html";
    } else if cfg!(target_os = "macos") && args_string.starts_with("hoptodesk://connect/") {
        if args_string.starts_with("hoptodesk://connect/") {
            let args_stringn = args_string.replace("hoptodesk://connect/", "");
            let mut iter = args_stringn.split('/');
            let id = iter.next().unwrap_or("").to_owned();
            let pass = iter.next().unwrap_or("").to_owned();
            let _teamid = iter.next().unwrap_or("").to_owned();
            let tokenexp = iter.next().unwrap_or("").to_owned();
            
            let args: Vec<String> = iter.map(|x| x.to_owned()).collect();

            if id.is_empty() {
                return;
            }

            if !tokenexp.is_empty() {
                std::fs::write(&Config::path("LastToken.toml"), tokenexp.clone())
                    .expect("Failed to write tokenexp to file");
            }
            
			frame.set_title(&id);
			frame.register_behavior("native-remote", move || {
				let handler = remote::SciterSession::new(
					"--connect".to_string(),
					id.clone(),
					pass.clone(),
					tokenexp.clone(),
					args.clone(),
				);
				#[cfg(not(any(feature = "flutter", feature = "cli")))]
				{
					*CUR_SESSION.lock().unwrap() = Some(handler.inner());
				}
				Box::new(handler)
			});	
        }

		page = "remote.html";
	} else {
        log::error!("Wrong command: {:?}", args);
        return;
    }
    #[cfg(feature = "packui")]
    frame.load_file(&format!("this://app/{}", page));
    #[cfg(feature = "inline")]
    {
        let html = if page == "index.html" {
            inline::get_index()
        } else if page == "cm.html" {
            inline::get_cm()
        } else if page == "install.html" {
            inline::get_install()
        } else {
            inline::get_remote()
        };
        frame.load_html(html.as_bytes(), Some(page));
    }
    #[cfg(all(not(feature = "inline"), not(feature = "packui")))]
    frame.load_file(&format!(
        "file://{}/src/ui/{}",
        std::env::current_dir()
            .map(|c| c.display().to_string())
            .unwrap_or("".to_owned()),
        page
    ));
    frame.run_app();
}

#[cfg(feature = "standalone")]
pub fn get_dll_bytes() -> &'static [u8] {
    DLL_BYTES
}

#[cfg(feature = "standalone")]
pub fn get_dllpm_bytes() -> &'static [u8] {
    DLL_BYTESPM
}	

#[cfg(feature = "standalone")]
pub fn get_dllph_bytes() -> &'static [u8] {
    DLL_BYTESPH
}	


struct UI {}

impl UI {
    fn recent_sessions_updated(&self) -> bool {
        recent_sessions_updated()
    }

    fn get_id(&self) -> String {
        ipc::get_id()
    }

    fn temporary_password(&mut self) -> String {
        temporary_password()
    }

    fn update_temporary_password(&self) {
        update_temporary_password()
    }

    fn permanent_password(&self) -> String {
        permanent_password()
    }

    fn set_permanent_password(&self, password: String) {
        set_permanent_password(password);
    }

    fn get_remote_id(&mut self) -> String {
        get_remote_id()
    }

    fn set_remote_id(&mut self, id: String) {
        set_remote_id(id);
    }

    fn goto_install(&mut self) {
        goto_install();
    }

    fn install_me(&mut self, _options: String, _path: String) {
        install_me(_options, _path, false, false, false);
    }
/*
    fn update_me(&self, _path: String) {
        update_me(_path);
    }
*/
    fn run_without_install(&self) {
        run_without_install();
    }

    fn show_run_without_install(&self) -> bool {
        show_run_without_install()
    }
    /*
        fn get_license(&self) -> String {
            get_license()
        }
    */
    fn get_option(&self, key: String) -> String {
        get_option(key)
    }

    fn get_local_option(&self, key: String) -> String {
        get_local_option(key)
    }

    fn set_local_option(&self, key: String, value: String) {
        set_local_option(key, value);
    }

    fn peer_has_password(&self, id: String) -> bool {
        peer_has_password(id)
    }

    fn forget_password(&self, id: String) {
        forget_password(id)
    }

    fn get_peer_option(&self, id: String, name: String) -> String {
        get_peer_option(id, name)
    }

    fn set_peer_option(&self, id: String, name: String, value: String) {
        set_peer_option(id, name, value)
    }
/*
    fn using_public_server(&self) -> bool {
        using_public_server()
    }
*/
    fn get_options(&self) -> Value {
        let hashmap: HashMap<String, String> =
            serde_json::from_str(&get_options()).unwrap_or_default();

        let mut m = Value::map();
        for (k, v) in hashmap {
            m.set_item(k, v);
        }
        m
    }

    fn test_if_valid_server(&self, host: String) -> String {
        test_if_valid_server(host)
    }

    fn get_sound_inputs(&self) -> Value {
        Value::from_iter(get_sound_inputs())
    }

    fn set_options(&self, v: Value) {
        let mut m = HashMap::new();
        for (k, v) in v.items() {
            if let Some(k) = k.as_string() {
                if let Some(v) = v.as_string() {
                    if !v.is_empty() {
                        m.insert(k, v);
                    }
                }
            }
        }
        set_options(m);
    }

    fn set_option(&self, key: String, value: String) {
        set_option(key, value);
    }

    fn get_config_option(&self, key: String) -> String {
        Config::get_option(&key)
    }

    fn set_config_option(&self, key: String, value: String) {
        Config::set_option(key, value);
    }

    fn requires_update(&self) -> bool {
        //log::info!("from config {} Vs from wire {}", crate::VERSION, Config::get_option("api_version"));
        get_version_number(crate::VERSION) < get_version_number(&Config::get_option("api_version"))
    }
	
	fn copy_text(&self, text: String) {
		copy_text(&text)
	}

    fn set_version_sync(&self) {
        set_version_sync()
    }

    fn install_path(&mut self) -> String {
        install_path()
    }

    fn get_socks(&self) -> Value {
        Value::from_iter(get_socks())
    }

    fn set_socks(&self, proxy: String, username: String, password: String) {
        set_socks(proxy, username, password)
    }

    fn is_installed(&self) -> bool {
        is_installed()
    }

    fn is_root(&self) -> bool {
        is_root()
    }

    fn is_release(&self) -> bool {
        #[cfg(not(debug_assertions))]
        return true;
        #[cfg(debug_assertions)]
        return false;
    }

    fn is_share_rdp(&self) -> bool {
        is_share_rdp()
    }

    fn set_share_rdp(&self, _enable: bool) {
        set_share_rdp(_enable);
    }

    fn is_installed_lower_version(&self) -> bool {
        is_installed_lower_version()
    }

    fn closing(&mut self, x: i32, y: i32, w: i32, h: i32) {
        crate::server::input_service::fix_key_down_timeout_at_exit();
		closing(x, y, w, h);
    }
	
    fn get_size(&mut self) -> Value {
        let s = LocalConfig::get_size();
        let mut v = Vec::new();
        v.push(s.0);
        v.push(s.1);
        v.push(s.2);
        v.push(s.3);
        Value::from_iter(v)
    }

    fn get_mouse_time(&self) -> f64 {
        get_mouse_time()
    }

    fn check_mouse_time(&self) {
        check_mouse_time()
    }

    fn get_connect_status(&mut self) -> Value {
        let mut v = Value::array(0);
        let x = get_connect_status();
        v.push(x.status_num);
        v.push(x.key_confirmed);
        v.push(x.id);
        v
    }

    #[inline]
    fn get_peer_value(id: String, p: PeerConfig) -> Value {
        let values = vec![
            id,
            p.info.username.clone(),
            p.info.hostname.clone(),
            p.info.platform.clone(),
            p.options.get("alias").unwrap_or(&"".to_owned()).to_owned(),
        ];
        Value::from_iter(values)
    }

    fn get_peer(&self, id: String) -> Value {
        let c = get_peer(id.clone());
        Self::get_peer_value(id, c)
    }

    fn get_fav(&self) -> Value {
        Value::from_iter(get_fav())
    }

    fn store_fav(&self, fav: Value) {
        let mut tmp = vec![];
        fav.values().for_each(|v| {
            if let Some(v) = v.as_string() {
                if !v.is_empty() {
                    tmp.push(v);
                }
            }
        });
        store_fav(tmp);
    }

    fn get_recent_sessions(&mut self) -> Value {
        // to-do: limit number of recent sessions, and remove old peer file
        let peers: Vec<Value> = PeerConfig::peers(None)
            .drain(..)
            .map(|p| Self::get_peer_value(p.0, p.2))
            .collect();
        Value::from_iter(peers)
    }

    fn get_icon(&mut self) -> String {
        get_icon()
    }

    fn remove_peer(&mut self, id: String) {
        PeerConfig::remove(&id);
    }

    fn remove_discovered(&mut self, id: String) {
        remove_discovered(id);
    }

    fn send_wol(&mut self, id: String) {
        crate::lan::send_wol(id)
    }

    fn new_remote(&mut self, id: String, remote_type: String, force_relay: bool) {
        new_remote(id, remote_type, force_relay)
    }

    fn is_process_trusted(&mut self, _prompt: bool) -> bool {
        is_process_trusted(_prompt)
    }

    fn is_can_screen_recording(&mut self, _prompt: bool) -> bool {
        is_can_screen_recording(_prompt)
    }

    fn is_installed_daemon(&mut self, _prompt: bool) -> bool {
        is_installed_daemon(_prompt)
    }

    fn get_error(&mut self) -> String {
        get_error()
    }

    fn is_login_wayland(&mut self) -> bool {
        is_login_wayland()
    }

    fn current_is_wayland(&mut self) -> bool {
        current_is_wayland()
    }

/*
    fn get_software_update_url(&self) -> String {
        get_software_update_url()
    }
*/
    fn get_new_version(&self) -> String {
        get_new_version()
    }

    fn get_version(&self) -> String {
        get_version()
    }

    fn get_fingerprint(&self) -> String {
        get_fingerprint()
    }

    fn get_app_name(&self) -> String {
        get_app_name()
    }

    fn get_software_ext(&self) -> String {
        #[cfg(windows)]
        let p = "exe";
        #[cfg(target_os = "macos")]
        let p = "dmg";
        #[cfg(target_os = "linux")]
        let p = "deb";
        p.to_owned()
    }

    fn get_software_store_path(&self) -> String {
        let mut p = std::env::temp_dir();
        let name = crate::SOFTWARE_UPDATE_URL
            .lock()
            .unwrap()
            .split("/")
            .last()
            .map(|x| x.to_owned())
            .unwrap_or(crate::get_app_name());
        p.push(name);
        format!("{}", p.to_string_lossy())
    }
	

    fn create_shortcut(&self, _id: String) {
        #[cfg(windows)]
        create_shortcut(_id)
    }

    fn discover(&self) {
        std::thread::spawn(move || {
            allow_err!(crate::lan::discover());
        });
    }

    fn get_lan_peers(&self) -> String {
        // let peers = get_lan_peers()
        //     .into_iter()
        //     .map(|mut peer| {
        //         (
        //             peer.remove("id").unwrap_or_default(),
        //             peer.remove("username").unwrap_or_default(),
        //             peer.remove("hostname").unwrap_or_default(),
        //             peer.remove("platform").unwrap_or_default(),
        //         )
        //     })
        //     .collect::<Vec<(String, String, String, String)>>();
        serde_json::to_string(&get_lan_peers()).unwrap_or_default()
    }

    fn get_uuid(&self) -> String {
        get_uuid()
    }

    fn open_url(&self, url: String) {
        #[cfg(windows)]
        let p = "explorer";
        #[cfg(target_os = "macos")]
        let p = "open";
        #[cfg(target_os = "linux")]
        let p = if std::path::Path::new("/usr/bin/firefox").exists() {
            "firefox"
        } else {
            "xdg-open"
        };
        allow_err!(std::process::Command::new(p).arg(url).spawn());
    }

    fn run_temp_update(&self) {
		#[cfg(windows)]
		{
			let exe_path = env::current_exe().expect("Failed to get current executable path").to_string_lossy().to_string();
			std::fs::write(&Config::path("UpdatePath.toml"), exe_path.clone()).expect("Failed to write update path");

			let mut tempexepath = std::env::temp_dir();
			tempexepath.push("HopToDesk-update.exe");
			log::info!("Saving update to: {:?}", tempexepath);
			let random_value = rand::random::<u64>().to_string();
			let url = format!("https://www.hoptodesk.com/update-windows?update={}", random_value);
			let rt = Runtime::new().unwrap();
			rt.block_on(async {
				log::info!("Downloading update...");
				let response = reqwest::get(url).await.expect("Error downloading update");
				let bytes = response.bytes().await.expect("Error reading token response");
				let _ = std::fs::remove_file(tempexepath.clone());
				let _ = std::fs::write(tempexepath.clone(), bytes);
				log::info!("Update saved.");
			});
		
			log::info!("Running update: {:?}", tempexepath.clone());
			let runuac = tempexepath.clone();
			if let Err(err) = crate::platform::windows::run_uac_hide(runuac.to_str().expect("Failed to convert executable path to string"), "--update") {
				log::info!("UAC Run Error: {:?}", err);
			} else {
				log::info!("UAC Run success");
			}
		
			std::process::exit(0);
		}
    }
	
    fn get_teamid(&self) -> String {
		use std::path::Path;
		if Path::new(&Config::path("TeamID.toml")).exists() {
			if let Ok(body) = std::fs::read_to_string(Config::path("TeamID.toml")) {
				return body;
			} else {
				eprintln!("Error reading file");
			}
		
		}
		String::from("none")
    }

	#[cfg(any(target_os = "android", target_os = "ios"))]
    fn change_id(&self, id: String) {
		reset_async_job_status();
        let old_id = self.get_id();
		change_id_shared(id, old_id);
    }

    fn post_request(&self, url: String, body: String, header: String) {
        post_request(url, body, header)
    }

    fn get_request(&self, url: String, header: String) {
        get_request(url, header)
    }
	
/*
    fn is_ok_change_id(&self) -> bool {
        hbb_common::machine_uid::get().is_ok()
    }
*/
    fn get_async_job_status(&self) -> String {
        get_async_job_status()
    }

    fn t(&self, name: String) -> String {
        crate::client::translate(name)
    }

    fn is_xfce(&self) -> bool {
        crate::platform::is_xfce()
    }

    /*
    fn get_api_server(&self) -> String {
        get_api_server()
    }
	*/
     fn has_hwcodec(&self) -> bool {
         has_hwcodec()
     }

    fn has_vram(&self) -> bool {
        has_vram()
    }
    
    fn get_langs(&self) -> String {
        get_langs()
    }

    fn video_save_directory(&self, root: bool) -> String {
        video_save_directory(root)
    }

    fn handle_relay_id(&self, id: String) -> String {
        handle_relay_id(&id).to_owned()
    }

    fn get_login_device_info(&self) -> String {
        get_login_device_info_json()
    }

    fn support_remove_wallpaper(&self) -> bool {
        support_remove_wallpaper()
    }

    fn has_valid_2fa(&self) -> bool {
        has_valid_2fa()
    }

    fn generate2fa(&self) -> String {
        generate2fa()
    }

    pub fn verify2fa(&self, code: String) -> bool {
        verify2fa(code)
    }

    fn generate_2fa_img_src(&self, data: String) -> String {
        let v = qrcode_generator::to_png_to_vec(data, qrcode_generator::QrCodeEcc::Low, 200)
            .unwrap_or_default();
        let s = hbb_common::sodiumoxide::base64::encode(
            v,
            hbb_common::sodiumoxide::base64::Variant::Original,
        );
        format!("data:image/png;base64,{s}")
    }
                
    fn get_custom_api_url(&self) -> String {
        if let Ok(Some(v)) = ipc::get_config("custom-api-url") {
            v
        } else {
            "".to_owned()
        }
    }

    fn set_custom_api_url(&self, url: String) {
        //ipc::set_config("custom-api-url", url);
		match ipc::set_config("custom-api-url", url) {
			Ok(()) => {},
			Err(e) => log::info!("Could not set custom API URL {e}"),
		}
		
    }
}

impl sciter::EventHandler for UI {
    sciter::dispatch_script_call! {
        fn t(String);
        //fn get_api_server();
        fn is_xfce();
        //fn using_public_server();
        fn get_id();
        fn temporary_password();
        fn update_temporary_password();
        fn permanent_password();
        fn set_permanent_password(String);
        fn get_remote_id();
        fn set_remote_id(String);
        fn closing(i32, i32, i32, i32);
        fn get_size();
        fn new_remote(String, String, bool);
        fn send_wol(String);
        fn remove_peer(String);
        fn remove_discovered(String);
        fn get_connect_status();
        fn get_mouse_time();
        fn check_mouse_time();
        fn get_recent_sessions();
        fn get_peer(String);
        fn get_fav();
        fn store_fav(Value);
        fn recent_sessions_updated();
        fn get_icon();
        fn install_me(String, String);
        fn is_installed();
        fn is_root();
        fn is_release();
        fn set_socks(String, String, String);
        fn get_socks();
        fn is_share_rdp();
        fn set_share_rdp(bool);
        fn is_installed_lower_version();
        fn install_path();
        fn goto_install();
        fn is_process_trusted(bool);
        fn is_can_screen_recording(bool);
        fn is_installed_daemon(bool);
        fn get_error();
        fn is_login_wayland();
        fn current_is_wayland();
        fn get_options();
        fn get_option(String);
        fn get_local_option(String);
        fn set_local_option(String, String);
        fn get_peer_option(String, String);
        fn peer_has_password(String);
        fn forget_password(String);
        fn set_peer_option(String, String, String);
        //fn get_license();
        fn test_if_valid_server(String);
        fn get_sound_inputs();
        fn set_options(Value);
        fn set_option(String, String);
        //fn get_software_update_url();
        fn get_new_version();
        fn get_version();
        fn get_fingerprint();
        //fn update_me(String);
        fn show_run_without_install();
        fn run_without_install();
        fn get_app_name();
        fn get_software_store_path();
        fn get_software_ext();
        fn open_url(String);
		fn run_temp_update();
		fn get_teamid();
        //fn change_id(String);
        fn get_async_job_status();
        fn post_request(String, String, String);
		fn get_request(String, String);
        //fn is_ok_change_id();
        fn create_shortcut(String);
        fn discover();
        fn get_lan_peers();
        fn get_uuid();
        fn has_hwcodec();
        fn has_vram();
        fn get_langs();
        fn video_save_directory(bool);
        fn handle_relay_id(String);
        fn get_login_device_info();
        fn support_remove_wallpaper();
        fn has_valid_2fa();
        fn generate2fa();
        fn generate_2fa_img_src(String);
        fn verify2fa(String);
        fn requires_update();
		fn set_version_sync();
		fn copy_text(String);
        fn get_config_option(String);
        fn set_config_option(String, String);
        fn get_custom_api_url();
        fn set_custom_api_url(String);
    }
}

impl sciter::host::HostHandler for UIHostHandler {
    fn on_graphics_critical_failure(&mut self) {
        log::error!("Critical rendering error: e.g. DirectX gfx driver error. Most probably bad gfx drivers.");
    }
}

use serde::Deserialize;
#[derive(Deserialize)]
struct Version {
    winversion: String,
    linuxversion: String,
    macversion: String,
    none: String,
}

async fn get_version_(refresh_api: bool) -> String {
	if refresh_api {
		hbb_common::api::erase_api().await;
	}
	
	match hbb_common::api::call_api().await {
        Ok(v) => {
			let body =  serde_json::from_value::<Version>(v).expect("Could not get api_version.");
           
            if cfg!(windows) {
				return body.winversion
            } else if cfg!(macos) {
                return body.macversion
            } else if cfg!(linux) {
                return body.linuxversion
            } else {
                return body.none
            }
        }
        Err(e) =>  {
            log::info!("{:?}", e);
             return "".to_owned();
        }
    };
}

use tokio::runtime::Runtime;

fn copy_text(text: &str) {
	let mut ctx = ClipboardContext::new().unwrap();
	ctx.set_contents(text.to_owned()).unwrap();
}


pub fn set_version_sync() {
    let rt = Runtime::new().unwrap();
    rt.block_on(async {
        Config::set_option("api_version".to_owned(), get_version_(true).await);
    });
}

#[tokio::main]
pub async fn set_version() {
    Config::set_option("api_version".to_owned(), get_version_(false).await)
}

#[cfg(not(target_os = "linux"))]
fn get_sound_inputs() -> Vec<String> {
    let mut out = Vec::new();
    use cpal::traits::{DeviceTrait, HostTrait};
    let host = cpal::default_host();
    if let Ok(devices) = host.devices() {
        for device in devices {
            if device.default_input_config().is_err() {
                continue;
            }
            if let Ok(name) = device.name() {
                out.push(name);
            }
        }
    }
    out
}

#[cfg(target_os = "linux")]
fn get_sound_inputs() -> Vec<String> {
    crate::platform::linux::get_pa_sources()
        .drain(..)
        .map(|x| x.1)
        .collect()
}

// sacrifice some memory
pub fn value_crash_workaround(values: &[Value]) -> Arc<Vec<Value>> {
    let persist = Arc::new(values.to_vec());
    STUPID_VALUES.lock().unwrap().push(persist.clone());
    persist
}

#[inline]
pub fn new_remote(id: String, remote_type: String, force_relay: bool) {
    let mut lock = CHILDREN.lock().unwrap();
    let mut args = vec![format!("--{}", remote_type), id.clone()];
    if force_relay {
        args.push("".to_string()); // password
        args.push("--relay".to_string());
    }
    let key = (id.clone(), remote_type.clone());
    if let Some(c) = lock.1.get_mut(&key) {
        if let Ok(Some(_)) = c.try_wait() {
            lock.1.remove(&key);
        } else {
            if remote_type == "rdp" {
                allow_err!(c.kill());
                std::thread::sleep(std::time::Duration::from_millis(30));
                c.try_wait().ok();
                lock.1.remove(&key);
            } else {
                return;
            }
        }
    }
    match crate::run_me(args) {
        Ok(child) => {
            lock.1.insert(key, child);
        }
        Err(err) => {
            log::error!("Failed to spawn remote: {}", err);
        }
    }
}

#[inline]
pub fn recent_sessions_updated() -> bool {
    let mut children = CHILDREN.lock().unwrap();
    if children.0 {
        children.0 = false;
        true
    } else {
        false
    }
}

pub fn get_icon() -> String {
    // 128x128
    #[cfg(target_os = "macos")]
    // 128x128 on 160x160 canvas, then shrink to 128, mac looks better with padding
    {
        "data:image/png;base64,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".into()
    }
    #[cfg(not(target_os = "macos"))] // 128x128 no padding
    {
        "data:image/x-icon;base64,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Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9FPTqzAAAAAAAAAAAAAAAARzw5LEY9OflGPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0o2L/8yX2//HoWn/0BISv9HOzb/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn5Rzw5LAAAAAAAAAAARz44fUY9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/SjYt/y5ne/8Bufv/AMP//zZZZv9JODD/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rz44fQAAAABNNDICSDs3w0U+Ov9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9JNzD/OFVf/wK19f8Av///F466/0c8N/9HPDf/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9FPjr/SDs3w000MgJFPToZRj057UY9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0c8N/9HOzf/EJvP/wC+//8GrOn/QEdJ/0g5M/9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj057UU9OhlHPjg8Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0o3Lv8wY3b/ALv//wC9//8ld5b/SzQr/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0c+ODxGPjlkRj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj04/0k4Mf8Uk8L/AL3//wOy9P88TVT/SDky/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y+OWRGPTmGRj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/SDoz/z1MUP8DsfH/ALz//w+d0f9HOzb/Rzw4/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9OYZGPTmlRj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Sjcv/zBjdf8Au///ALz//xeOuf9KNzD/Rj05/0Y9Of9GPTn/Rj05/0Y8OP9IOTP/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9OaVGPDm8Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/SjUt/yhyjf8Avf//ALz//xuIr/9KNi7/Rj05/0Y9Of9GPTn/Rj04/0RAPf88TlT/SDkz/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y8ObxGPTnIRj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/SzUs/yR6mf8Avf//ALz//xmKtP9KNy//Rj05/0Y9Of9GPTn/STgx/zZYZf8EtfP/Ml9w/0s1LP9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9OchGPTnSRj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/SzUs/yN7mv8Avf//ALv//xSUxP9JOTL/Rj04/0Y9Of9GPTn/SDkz/zxPVf8AuPr/A7f2/zJgcf9JNy//Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9OdJGPTncRj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/SzUs/ydzjv8AvP//ALj9/wmm4f9DQUD/Rzs2/0Y9Of9GPTn/Rj04/0k5Mf8Zi7b/AMD//weq5v9CRET/Rzs1/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9OdxGPTnmRj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Sjcv/zBjdf8Au///ALb5/wC5/f80XWz/SzYu/0Y9Of9GPTn/Rj05/0k5Mv88T1b/ArP1/wC9//8yYHD/Sjcu/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9OeZGPTnoRj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/SDkz/z1OVP8Cs/T/ALf7/wC7//8Tlsb/STgx/0c7Nf9GPTn/Rj05/0k3L/84VmL/Arb5/wC9//8dhKr/SzQr/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9OehGPTnqRj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rjw4/0g6NP8SmMr/ALz//wC1+P8Au///IX2g/0g5Mv9LNSv/SzUs/0k3L/8VksH/ALv//wC3+/8EsvH/OlFY/0o2Lv9GPTr/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9OepGPTnqRj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0c8N/9HOzX/SDkz/0g5M/9IOjT/Rzs2/0s1LP8uZ3z/ALv//wC1+P8Atvn/ALr//xGZy/8qbYb/LmZ6/xeQvP8AuP7/ALX5/wC1+P8Auv//DaLZ/0BHSP9KNi3/Rj05/0Y9Of9GPTn/Rj05/0c8OP9GPTj/Rj05/0Y9Of9GPTn/Rj05/0Y9OepGPTnoRj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9IOTP/SjUt/0Y9Of9BRkf/PU1S/zxPVf8/Sk3/QUVF/0U/PP9IOjX/EZnM/wC6/v8Atfj/ALX4/wC6//8AvP//ALz//wC7//8Atfj/ALX4/wC1+P8Atfj/ALv//wul3v83WGP/SjYt/0s0LP9KNi3/STcv/0Y8OP9IOTP/Rzs2/0Y9Of9GPTn/Rj05/0Y9OehGPTnmRj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rzo1/0o2Lf88TVP/I3qZ/w6f1f8HrOj/A7Hy/wKy9P8EsO7/B6rm/wuk3P8RmMn/Cabh/wC3+v8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC6//8Btff/GYu1/yV4lv8ggaP/Goqz/w6g1v8biK//RT88/0c8N/9GPTn/Rj05/0Y9OeZGPTncRj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9KNi//Q0JB/yB/of8EsvL/AL3//wC6//8AuPz/ALf7/wC3+v8At/v/ALj8/wC5/v8Auv//ALf8/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atvr/ALv//wC9//8Avf//ALz//wC6//8Awv//KXCK/0o2Lv9GPTn/Rj05/0Y9OdxGPTnSRj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rzw4/0s1LP81Wmf/DKXd/wC9//8At/z/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC4/f8BuPr/MGJ1/0k4Mf9GPTn/Rj05/0Y9OdJGPTnIRj05/0Y9Of9GPTn/Rj05/0Y9Of9HOzb/SjYv/ylwif8Ctvn/ALr//wC1+P8Atfj/ALf7/wC6//8Au///ALj9/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC3+/8Atvr/ALX4/wC1+P8Atfj/ALj9/wO09f8xYXP/Sjcv/0Y9Of9GPTn/Rj05/0Y9OchGPDm8Rj05/0Y9Of9GPTn/Rj05/0c7Nv9JODL/IH6h/wC8//8At/z/ALX4/wC4/v8Avf//A7Hx/w+c0P8UlMP/Cafi/wC7//8AuPz/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atvr/AL3//wWt7P8Eru3/ALb6/wC1+P8Auf7/Abj7/zJgcf9LNCv/Rj05/0Y9Of9GPTn/Rj05/0Y8ObxGPTmlRj05/0Y9Of9GPTn/Rzs1/0g5M/8chaz/AL3//wC2+v8AuP3/AL3//wqn4f8ld5T/PUxR/0c7Nv9JODH/REFA/yhyjv8EsPD/ALz//wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC6//8Btvn/In2e/zRda/8HrOj/ALj8/wC9//8FsfD/LmZ7/0o2Lf9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9OaVGPTmGRj05/0Y9Of9HOzf/STgx/x2Fq/8Avv//ALf8/wC9//8Iqub/KHKM/0NBQP9LNS3/SDkz/0Y8OP9GPTn/Rzs1/0s1K/88TlX/EprL/wC+//8At/z/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8Atvr/AL3//wmo4v81W2n/Ti8j/xuGrf8Avv//ALr//x6Eqf89S1D/SjYv/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9OYZGPjlkRj05/0Y8OP9IOjT/IX2f/wC9//8AvP//ArX3/x+BpP9BRUb/SzUs/0c7Nf9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9JODH/Rzw3/yV3lf8EsvL/AL3//wC4/f8Atvn/ALX4/wC1+P8Atvr/ALj9/wC9//8Bt/j/G4qy/0FFRv9LNCv/Q0JB/wWu7f8Awv//HYWq/0o1Lf9JODL/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y+OWRHPjg8Rj05/0g6NP9ASEn/ALf6/wDA//8Rmcz/OFZg/0o1LP9IOjT/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rzs2/0o1LP88T1X/HYWr/wer6P8At/v/ALv//wC7//8Btfj/Cank/x2FrP84VmD/STcw/0g5M/9GPTj/RT47/yZ0kf8qbYb/Rzs1/0c7Nv9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0c+ODxFPToZRj057UY8OP9GPDj/JnaT/yd0kP9HPDb/STcw/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9IOTL/STYv/0JFRP84VmH/MGN3/zFhc/85U1z/Q0NC/0o2Lv9JODH/Rj05/0Y9Of9GPTn/Rj05/0o3L/9KNi//Rjw4/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj057UU9OhlNNDICSDs3w0U+Ov9GPTj/STcv/0o2L/9HPDf/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0c7Nf9JODH/Sjcv/0o3L/9IOTL/Rzs2/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9FPjr/SDs3w000MgIAAAAARz44fUY9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rz44fQAAAAAAAAAARzw5LEY9OflGPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn5Rzw5LAAAAAAAAAAAAAAAAEU9OrNGPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9FPTqzAAAAAAAAAAAAAAAAAAAAAEc8OTtGPTn9Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of1HPDk7AAAAAAAAAAAAAAAAAAAAAAAAAABHPjmWRj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9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ODtFPDmcRTw330Y8Of9FPDn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/RTw5/0Y8Of9FPDffRTw5nEU9ODtGPTgARz01AEY9OQBGPTkART04AEY9OABGOzoARjs6AEU9OgpGPDiWRj059kU9Of9GPTj/Rj05/kY9Of1FPTn+Rjw4/kU9Of5GPTn+Rj05/kY9Of5GPTn+Rj05/kY9Of5FPTn+Rjw4/kU9Of5GPTn9Rj05/kY9OP9FPTn/Rj059kY9OJZGPToKRjw5AEY9OQBFPTgART04AkU8OAFFPDgORjw3uEU7Of9FPTj/Rjw5+kY8Of1GPDn+RT05/0Y9Of5GPTn/Rj05/0Y8OP9FPTn/RT05/0Y8OP9GPDj/RT05/0Y8OP9GPTn/Rj05/kU9Of9GPDn+Rjw5/UY8OfpFPTj/RT05/0U8N7hFPTgORTw4AUU9OAFGPTgERTw4AEU8OJdFPDj/Rjw5+kY9OP1FPTn/Rj04/kY9Of5GPTn/Rj05/0Y9Of9GPDj/RT05/0Y8OP9FPTn/RT47/0U9Ov9GPDj/RT05/0Y8OP9GPTn/Rj05/0Y9Of5GPTj+RT05/0U9OP1GPTn6RDw4/0U8OJdEOzgARTw5A0M8OgBDPDo8RTw5+0U9OPxFPTn9Rj05/0Y9Of5GPTn/Rj05/0Y9Of9GPTn/Rj05/0U9Of9GPDj/RT05/0U8OP9KMyv/Rzcx/0U9Of9GPDj/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn+Rj05/0U9Of1FPTj8RTw5+0M8OjlJQjQARD45AUU8OJ1EPDn/Rj04+0U8Of5GPTj+Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rjw4/0U9Of9FOzb/Rzgy/yN7lf8wYnL/STYv/0U9Of9GPDj/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9FPTj+RT05/kY9OPtEPDn/RTw4nD43PwBLNjJHRjw31kU8Of5FPDf8RT05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj04/0Y9Of9FPDj/RT05/0ozK/8ffaD/AMj//y9jdP9KNCz/RD47/0Y7N/9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9FPTn/RTw3/EU8Of5GPDfgNihICUU8OJlFPDj3RT05/0Y8OP5FPTn+Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rjw4/0Q+O/9KNS3/NVhl/wDC//8bhaz/SjEo/0Q9Ov9FPDj/Rjw4/0U9Of9GPDj/Rj05/0Y9Of9GPTn/Rj05/0U9Of5GPDj+RT05/UU8OP9KQDM4RT04vEY9Of9GPDn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y8OP9FOzf/RD06/0k2Lf8Vkb7/AL38/ztOVf9JNi7/RD47/0Y8OP9FPTn/Rjw4/0U9Of9GPDj/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPDn8RTs5/kU8OW1FPTnQRj05/kU9Of9GPTn+RT05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rjw3/0U9Of9HOTL/PUhL/wG19v8Lotn/Rjs3/0U8N/9EPTn/Rjo2/0Y7Nv9FPjr/Rjw4/0Y9Of9GPDj/Rj05/0Y9Of9FPTn/Rj05/kU9Of1GPDj+RT05m0Y8OOFGPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GOzj/RD47/0k1LP80Wmj/AMH//xOVw/9INzH/RDw5/0U8N/9DPz3/Qz49/0c4Mf9FPTr/Rjw4/0U9Of9GPDj/Rj05/0Y9Of9GPTn/Rjw5/kU9Of9GPDi/Rjw56UY9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y8OP9EPTv/SjQq/zBhcv8Awv7/EpbG/0c4Mv9EPjr/STUu/zZWYv8OodT/QERF/0c5Mv9EPTr/Rjw4/0Y9Of9GPTn/Rj05/0Y9Of9GPDn9Rjw5/0Y8OdFGPTnvRj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rjw4/0U9Of9GPDj/Rjs3/0Q+O/9KMyv/MV9v/wC///8Jo97/RD47/0U9OP9FOzb/Q0E//wev6P8PnM//Rjk0/0U9OP9FPDj/RTw4/0Y8OP9GPDj/Rjw4/0Y9Of5GPTj/Rj053UY9OfRGPTn/Rj05/0Y9Of9GPTn/Rj05/0Y8OP9FPTn/Rjw4/0Y8OP9FPDj/RD47/0g2MP85Ulv/ALb4/wC5+/81WWb/TDEn/0FCQf9LMSb/J3CM/wDC//86UFf/STYv/0Q+O/9GOzf/Rjw4/0Y8OP9GPDj/Rjw4/kU9Of9GPDjpRj059UY9Of9GPTn/Rj05/0U8OP9FPTn/Rj05/0Y8OP9FPjr/RD88/0U9Of9FPTj/RDw4/0M/Pf8KpN3/AL///w+c0P9BQkL/SzEn/0c3MP8UksH/AML9/x6ApP9MMCX/RD06/0M/PP9EPzz/RD88/0U+Ov9FPDj+Rj04/0U9OetGPTn1Rj05/0Y8OP9GPDj/Rj05/0Y8OP9FPTn/RT06/0g3MP9KMyn/STYu/0g3Mf9HODH/Tiwe/yxqgf8Auv3/ALn9/wmn4v8gf6D/FJTE/wC4/v8AtPf/ALn9/yhviP9LMij/SjIp/0ozKf9KMin/SDYv/0U9Of5FPDj/Rjw460Y9OfRGPDj/RT05/0U9Of9GPDj/RD47/0Y6NP9JMyr/O05V/yJ5mf8Wj7z/EpTF/xWRvv8bhaz/HIOp/wGy9P8AtPf/ALf8/wC///8Au///ALT3/wC09/8Atfn/ALr+/xiJs/8vZXj/KW6F/x6BpP80Wmb/SDcx/kQ9Ov9FPDjpRj0470U9Of9GPDj/Rjw4/0Q/PP9JNS7/RD47/x9/ov8DsvL/ALz//wC7//8Auf//ALz//wC9//8AvP//ALT4/wC09/8AtPb/AbL0/wCz9f8AtPf/ALX4/wCz9v8AsvT/ALv//wC9//8Auf3/AMf//w6g0/9HODP+Rjw3/0Q9Od1GPDnpRjw4/0U7N/9EPjv/SjMr/ztNU/8NoNb/AL7//wC6//8Au///ALz//wC5//8BsfP/AbL0/wCz9v8Atfj/ALX4/wC1+P8Atfj/ALX4/wC1+P8As/b/ALX4/wC7//8As/b/ALDz/wC3+/8Fr+3/M1pp/0c4Mv1FPTn/RTw40UY7OOFFPDj/RD06/0ozKv82VWD/Bq7p/wDB//8Auv//DKLZ/x99oP8ncY3/EZjK/wC9//8Atvr/ALL0/wC09/8Atfj/ALX4/wC09/8As/b/ALLz/wC5/v8Atvr/FJK//wOs7P8Av///BLXy/zVYZf9LMSb/RD46/kU8OP9GPDi/Rjw30EQ9Ov5JNC3/N1Rf/gO18/8Awv//DaDV/y1ofP9EPjz/SjMq/0szKP9HODL/KHCK/wO08/8AvP//ALX3/wCy9f8AsvT/ALP1/wC2+v8Avv//CKvl/zdUYP8obob/AMX//xWUv/86Tlb/STYu/0Q+O/5FPDj9Rjw5/kU9OZtCQT+8SDcx/zxKT/8DuPP/BLbw/yVzj/9FPTn/SjIp/0Y6Nf9EPjv/Qz48/0U7N/9LMij/Ok9X/xWTwv8At/r/AL3//wC9//8Au///Bq/r/xyFq/8/Rkj/Tiwe/xeMtv8Wk7v/Rzgy/0k1LP9EPjv/Rjw4/0Y8OPxFOzn+RTw5bUQ/PplHNzH3PEtQ/x2Hqf46T1b+SzEn/0Y7Nv9EPjv/RT06/0U9Of9GPDj/RT06/0Q+PP9INi//Rzgx/zZVX/8qbof/JnOQ/y5kd/8+SU3/SjQs/0c5M/9EPjv/Qz8+/0Y5NP9GOzX/RD47/0U7N/5GPDj+RT05/UU8OP9KQDM4TDQvR0U9OdZGOjT+SjIq/Eg4Mf9EPjv/RTw5/0Y8OP9FPTn/Rjw4/0U9Of9GPTn/Rjw4/0U9Ov9FPDj/STYu/0szKv9LMyn/STQr/0g4Mf9EPjr/RT05/0Y9OP9HOzX/Rjw4/0U9Ov9GPDj/RT05/0U8N/xFPDn+Rjw34DYoSAlEPjkBRTw4nUQ9Of9FPjv7RT46/kY9Of5GPTj/RT05/0Y8OP9GPTn/Rjw4/0U9Of9FPTn/Rjw4/0U9Of9FPjv/RT47/0U+O/9FPjv/RT46/0U9Of9GPTj/Rjw4/0U9Of9FPTn/Rjw4/0U9Of5FPDn+Rj04+0Q8Of9FPDicPjc/AEQ8OgBDPDo8RTw4+0U8OPxFPDj9Rj05/0Y8OP5GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPDj/RT05/0Y8OP9GPDj/Rjw4/0Y8OP9GPDj/RT05/0Y8OP9GPTn/Rj05/0Y9Of9GPTn+Rj05/0U9Of1FPTj8RTw5+0M8OjlJQjQARTw4BEQ7OABFPDiXRDw4/0Y9OfpFPTj9RT05/0Y9OP5GPTn+Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn+Rj04/kU9Of9GPTj9Rjw5+kU8OP9FPDiXRTw4AEU8OQNFPTgCRTw4AUU9OA5FPDe4RT05/0U9OP9GPDn6Rjw5/UY8Of5FPTn/Rj05/kY9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn+RT05/0Y8Of5GPDn9Rjw5+kU9OP9FOzn/Rjw3uEU8OA5FPDgBRT04AUU9OABGPTkARjw5AEY9OgpGPTiWRj059kU9Of9GPTj/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9OP9FPTn/Rj059kY8OJZFPToKRjs6AEY7OgBGPTgART04AEY9OQBGPTkARz01AEY9OABFPTg+RTw5nUY8N9ZGPDn2RT05/0Y9Of5GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/kU9Of9GPDn2Rjw31kU8OZ1FPTg+Rj04AEY9NQBGOzoARjs6AEY+OAAAAAAARj05AEY9OQAAAP8DRj05BEQ9NwBCPzkARjs3SkU7OJZEPTi7RT050EY8OOFFPTnpRj0570Y9OfVGPTn2Rj059kY9OfVGPTnvRT056UY8OOFFPTnQRD04u0U7OJZGOzdKQj85AEQ9NwBGPDkEAAD/A0Y8OgBGPDoAAAAAAP+AAf/8AAA/8AAAD+AAAAfAAAADwAAAA4AAAAGAAAABAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAYAAAAGAAAABwAAAA8AAAAPgAAAH8AAAD/wAAD//AAD/KAAAABgAAAAwAAAAAQAgAAAAAABgCQAAAAAAAAAAAAAAAAAAAAAAAEY9OQBGPTgARj05A0Y9NwBGPTkASDs4FEY7N1xFPTiYRTs5xEY9OdpFPTnoRj0570Y9Oe9FPTnoRj052kU7OcRFPTiYRjs3XEg7OBRGPTkARj03AEY9OQNGPTgART04AEY9OQBGPTgBRj05AEY9NytGPTmlRTw47UY8OP9FPTn/Rjw5/0Y9Of9GPTn/Rj05/0Y9Of9GPTn/Rj05/0Y8Of9FPTn/Rjw4/0U8OO1GPTmlRj03K0Y9OQBGPTgBRD03AEU9OQRGPDkART04UUU8OPJGPTn/Rj05/0Y9OfxGPTj9RT05/kU9Of5FPTn+Rjw4/kU9Ov5FPTn+Rjw4/kU9Of5GPDj9Rj05/EY9Of9GPTn/RTw48kU9OFFGPDkART04A0Y8OAJGPDkuRT048UU8Of9GPTn5RTw4/UY8OP5FPTn+RT05/kY8OP9GPDj/RT46/0c5M/9GPDf/RT05/0Y8OP5FPTn+Rjw4/kU8OP1GPTn5RTw5/0U9OPFGPDksRjw4AUM/OABFPDioRTw4/0Y9OflGPTn/Rj05/kY9Of5GPTn/Rjw4/0U8OP9EPTr/SDcw/z5JS/9DQT//RTs2/0U8OP9GPDj/Rj05/kY9Of5GPTn/Rj05+UU8OP9FPDipRTw5AEM+N1FFPDjnRDw5/UU8OP1GPTn/Rj05/0Y9Of9FPTn/RTw4/0Q+O/9JNCz/MV9v/wet5f9ARUX/Rzk0/0Q9Of9GOzf/Rj05/0Y9Of9GPTn/RTw4/UQ8Of5FPDjxRzo2FEY8OL9FPTn7RT04/0Y8OP5GPTn+Rj05/0Y9Of9GPDj/RD05/0Y7Nv9EPjz/BLLw/yR3lP9LMin/RD47/0U8OP9GPDj/RT05/0Y8OP9GPTn+Rjw4/kU9OPxFPDj/Rjw3XUU9ONtGPDj/Rjw5/0Y9Of9GPTn/Rj05/0Y9Of9FPDf/RD48/0ozKf8pbYb/Abb4/0NAP/9GOjT/RTw4/0U8OP9FPTr/Rjw4/0U9Of9GPDj/Rj05/0Y9OfxFPDn+RT04mUY8OelFPTn+RT05/0Y8OP5GPTn/Rj05/0Y9Of9EPTj/RD47/0ozKf8Wj7v/Cqbd/0c4Mv9FPDj/Qz88/0Y6Nf9GOjX/RTw5/0Y8OP9GPTn/Rjw4/kU8Of1GPTj+Rjw5xEY8OPFGPTn/Rj05/0Y9Of9GPTn/RTw4/0Y9Of9FPDn/RT06/0k1LP8Tl8b/CqXe/0Y6Nf9INzD/N1Re/xaRvP9EPTn/RTs3/0U8OP9FPDn/Rjw4/0Y8Of5FPDn/Rjw420Y9OfZGPTn/Rj05/0Y8OP9FPDj/Rj04/0U9Of9EPTn/RD48/0k0LP8Xjbj/Arf3/0JCQv9HOTP/Rzk0/wyj2f8kd5T/SzIo/0Q+PP9GPDj/RT05/0Y9OP1GPDj/Rj056EY9OflGPDj/RTw4/0U9Of9GPDj/RT06/0U+O/9GOjX/Rjs1/0wwJf8pboj/AMb//yB+oP9KMyn/STUs/w+c0P8Lo9r/STUt/0c5M/9EPTn/Rjw3/0Y8OP5FPTn/Rj0470Y9OflFPTn/Rj05/0Y8OP9FPjv/Rzgx/0k0K/9CQUD/PUpP/0BERP89Sk7/Bqvo/wC8//8Sl8f/EZnK/wC3/P8Auv//HIat/0U7N/9HODH/RD07/0Y5NP5FPTn/Rjw370U9OfZGPDj/RTw4/0Q+O/9KNCv/PUlO/xuHrf8HqeX/A6/v/wOv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".into()
    }
}