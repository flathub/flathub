use {
    glib::source::timeout_add_local,
    gtk::prelude::*,
    std::{
        cell::RefCell,
        fs::{self, File},
        io::{ErrorKind, Read, Write},
        rc::Rc,
        sync::{
            atomic::{AtomicBool, AtomicUsize, Ordering},
            Arc,
        },
    },
};

macro_rules! clone_push {
    ($item:ident => $closure:expr) => {
        let item = $item.clone();
        $closure(item);
    };
}

pub mod element_ids {
    pub const APP_WINDOW: &'static str = "aw";
    pub const HBAR: &'static str = "hbar";
    pub const FCN: &'static str = "fcn";
    pub const FCB: &'static str = "fcb";
    pub const START: &'static str = "start";
    pub const RELOADER: &'static str = "reloader";
    pub const LSBLKCBT: &'static str = "lsblkCbt";
    pub const PBAR: &'static str = "pbar";
    pub const PBAR_REVEALER: &'static str = "pbarRevealer";
}

fn get_element_by_id_from_builder<T: glib::object::IsA<glib::object::Object>>(
    builder: &gtk::Builder,
    id: &str,
) -> T {
    builder
        .get_object::<T>(id)
        .expect(&format!("Unknown element: {}", id))
}

pub struct UI {
    builder: gtk::Builder,
    hbar: gtk::HeaderBar,
    fcn: gtk::FileChooserNative,
    fcb: gtk::Button,
    start: gtk::Button,
    reloader: gtk::Button,
    lsblk_cbt: gtk::ComboBoxText,
    pbar_revealer: gtk::Revealer,
    pbar: gtk::ProgressBar,
}

impl UI {
    fn get_element_by_id<T: glib::object::IsA<glib::object::Object>>(&self, id: &str) -> T {
        get_element_by_id_from_builder(&self.builder, id)
    }

    fn get_selected_file(&self) -> Option<std::path::PathBuf> {
        let selected_file = self.fcn.get_filename();
        dbg!(&selected_file);
        selected_file
    }

    fn get_selected_device(&self) -> Option<crate::aux::backend::BlockDrive> {
        // Combo box text only stores a Gstring (Device ID)
        // Search through the list of devices from udisks2 again
        // and find the device with matching device ID

        let selected_device = match self.lsblk_cbt.get_active_text() {
            Some(txt) => {
                let txt = txt.as_str();
                dbg!(&txt);
                for disk in crate::aux::backend::get_disks().expect("Error reading disks") {
                    if &disk.id == txt {
                        return Some(disk);
                    }
                }
                dbg!("No matching device found. Must reload.");
                None
            }
            None => {
                dbg!("lsblk_cbt is returning nothing");
                None
            }
        };
        selected_device
    }

    pub fn new(app: &gtk::Application) -> Rc<RefCell<Self>> {
        let builder = gtk::Builder::from_string(include_str!("../../resources/nixwriter.glade"));
        let builder_clone = builder.clone();
        let result = Rc::new(RefCell::new(Self {
            builder: builder,
            hbar: get_element_by_id_from_builder(&builder_clone, element_ids::HBAR),
            fcn: get_element_by_id_from_builder(&builder_clone, element_ids::FCN),
            fcb: get_element_by_id_from_builder(&builder_clone, element_ids::FCB),
            start: get_element_by_id_from_builder(&builder_clone, element_ids::START),
            reloader: get_element_by_id_from_builder(&builder_clone, element_ids::RELOADER),
            lsblk_cbt: get_element_by_id_from_builder(&builder_clone, element_ids::LSBLKCBT),
            pbar_revealer: get_element_by_id_from_builder(
                &builder_clone,
                element_ids::PBAR_REVEALER,
            ),
            pbar: get_element_by_id_from_builder(&builder_clone, element_ids::PBAR),
        }));
        result
            .borrow()
            .get_element_by_id::<gtk::ApplicationWindow>(element_ids::APP_WINDOW)
            .set_application(Some(app));

        // let rc = result.clone();
        // Self::set_fcb(rc);
        clone_push!(result => Self::set_fcb);
        clone_push!(result => Self::set_lsblk_cbt);
        clone_push!(result => Self::set_reloader);
        clone_push!(result => Self::set_start);        
        result
    }

    fn set_fcb(this: Rc<RefCell<Self>>) {
        let fcn = this.borrow().fcn.clone();
        let fcb = this.borrow().fcb.clone();
        let start = this.borrow().start.clone();
        fcb.connect_clicked(move |b| {
            let resp = fcn.run();
            if resp == gtk::ResponseType::Accept {
                b.set_label(
                    fcn.get_filename()
                        .expect("No file selected")
                        .file_name()
                        .expect("Error selecting file")
                        .to_str()
                        .expect("Invalid file name"),
                );
                let fc = this.borrow().get_selected_file();
                let dc = this.borrow().get_selected_device();
                start.set_sensitive(fc.is_some() && dc.is_some());
            }
        });
    }

    fn set_lsblk_cbt(this: Rc<RefCell<Self>>) {
        let cbt = this.borrow().lsblk_cbt.clone();
        for ddev in crate::aux::backend::get_disks().expect("Error reading disks") {
            cbt.append_text(&ddev.id);
        }
        let start = this.borrow().start.clone();
        cbt.connect_changed(move |_| {
            let (device_chosen, file_chosen) = (
                this.borrow().get_selected_device().is_some(),
                this.borrow().get_selected_file().is_some(),
            );
            start.set_sensitive(device_chosen && file_chosen);
            dbg!("From set_lsblk_cbt", device_chosen, file_chosen);
        });
    }

    fn set_reloader(this: Rc<RefCell<Self>>) {
        let reloader = this.borrow().reloader.clone();
        let start = this.borrow().start.clone();
        let cbt = this.borrow().lsblk_cbt.clone();
        reloader.connect_clicked(move |_| {
            cbt.remove_all();
            start.set_sensitive(false);
            for ddev in crate::aux::backend::get_disks().expect("Error reading disks") {
                cbt.append_text(&ddev.id);
            }
        });
    }

    fn set_start(this: Rc<RefCell<Self>>) {
        let revealer = this.borrow().pbar_revealer.clone();
        let start = this.borrow().start.clone();
        let pbar = this.borrow().pbar.clone();
        let hbar = this.borrow().hbar.clone();
        start.connect_clicked(move |s| {
            // deactivate
            s.set_sensitive(false);
            // reveal pbar
            revealer.set_reveal_child(true);
            // start task
            let (input, output_drive) = (
                this.borrow()
                    .get_selected_file()
                    .expect("No files selected"),
                this.borrow()
                    .get_selected_device()
                    .expect("No device selected"), // .parent
                                                   // .device,
                                                   // keep this as is for debug purposes
            );
            dbg!("File path:", &input);
            let total_len = Arc::new(AtomicUsize::new(
                fs::metadata(&input.as_path()).unwrap().len() as usize, // requires root in flatpak builds
            ));
            let written = Arc::new(AtomicUsize::new(0));
            let finished = Arc::new(AtomicBool::new(false));
            let mut input = File::open(input).unwrap();
            dbg!("output_drive.parent.device = ", &output_drive.block);
            let mut output = output_drive.open().expect("Could not open output file.");

            dbg!("---", "Starting operation: ", &input, &output, "---");
            let wrtn1 = Arc::clone(&written);
            let finished_clone = Arc::clone(&finished);
            std::thread::spawn(move || {
                let mut buf = vec![0; crate::aux::consts::BS];
                dbg!("R/W loop started");
                loop {
                    let read_len = match input.read(&mut buf) {
                        Ok(0) => {
                            dbg!("Nothing written");
                            break;
                        }
                        Ok(new_read_len) => new_read_len,
                        Err(ref e) if e.kind() == ErrorKind::Interrupted => {
                            // something went wrong
                            dbg!(e);
                            continue;
                        }
                        Err(e) => {
                            let msg = format!("Error reading file: {:?}", e);
                            println!("{}", &msg);
                            panic!();
                        }
                    };
                    output
                        .write_all(&buf[..read_len])
                        .expect("File writing failed");
                    wrtn1.fetch_add(read_len, Ordering::SeqCst);
                }
                finished_clone.store(true, Ordering::SeqCst);
                dbg!("R/W loop finished");
            });
            // glib timeout
            let finished_clone2 = Arc::clone(&finished);
            let tl = Arc::clone(&total_len);
            let wrtn2 = Arc::clone(&written);
            let pbar = pbar.clone();
            let ss = s.clone();
            let rvlr = revealer.clone();
            let hbar = hbar.clone();
            timeout_add_local(10, move || {
                let progress = wrtn2.load(Ordering::SeqCst);
                let tl = tl.load(Ordering::SeqCst);
                if progress < tl {
                    let frac = (progress as f64) / (tl as f64);
                    pbar.set_fraction(frac);
                    hbar.set_subtitle(Some(&format!("Writing in progress: {:.2}%", 100. * frac)));
                    Continue(true) // glib::prelude::Continue
                } else {
                    if finished_clone2.load(Ordering::SeqCst) {
                        rvlr.set_reveal_child(false);
                        hbar.set_subtitle(None);
                        // reactivate start
                        ss.set_sensitive(true);
                        // try notify
                        crate::aux::backend::end_notify().unwrap();
                        Continue(false)
                    } else {
                        hbar.set_subtitle(Some(
                            "Your device is still reading the buffer. Please be patient.",
                        ));
                        pbar.pulse();
                        Continue(true)
                    }
                }
            });
        });
    }

    pub fn show_all(&self) {
        self.get_element_by_id::<gtk::ApplicationWindow>(element_ids::APP_WINDOW)
            .show_all();
    }
}
