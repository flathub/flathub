use souvlaki::{MediaControls, PlatformConfig};
use std::thread::sleep;
use std::time::Duration;

fn main() {
    {
        #[cfg(not(target_os = "windows"))]
        let hwnd = None;

        #[cfg(target_os = "windows")]
        let hwnd = {
            use raw_window_handle::Win32WindowHandle;

            let handle: Win32WindowHandle = unimplemented!();
            Some(handle.hwnd)
        };

        let config = PlatformConfig {
            dbus_name: "my_player",
            display_name: "My Player",
            hwnd,
        };

        let mut controls = MediaControls::new(config).unwrap();

        controls.attach(|_| println!("Received message")).unwrap();
        println!("Attached");

        for i in 0..5 {
            println!("Main thread sleeping:  {}/4", i);
            sleep(Duration::from_secs(1));
        }
    }
    println!("Dropped and detached");
    sleep(Duration::from_secs(2));
}
