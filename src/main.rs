mod aux;
mod frontend;

use gio::prelude::*;

fn main() {
    let app = gtk::Application::new(Some(aux::consts::APP_ID), Default::default())
        .expect("Could not initialize the Gtk application");
    app.connect_activate(|application| {
        let ui = frontend::UI::new(application);
        ui.borrow().show_all();
    });
    app.run(&[]);
}
