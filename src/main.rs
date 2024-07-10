extern crate adw;
extern crate async_channel;
extern crate gtk4;
extern crate regex;

use std::cell::RefCell;
use std::collections::HashMap;
use std::rc::Rc;

use adw::gio::{ActionGroup, ActionMap};
use adw::subclass::prelude::*;
use adw::{gio, PreferencesGroup};
use adw::{prelude::*, ActionRow, Application, ApplicationWindow, EntryRow, Window};
use async_channel::{unbounded, Receiver, Sender};
use gtk4::glib::subclass::InitializingObject;
use gtk4::glib::{self, object_subclass, GString, Object};
use gtk4::{
    Accessible, Buildable, Button, ButtonsType, CompositeTemplate, ConstraintTarget, DialogFlags,
    FileChooserNative, FileFilter, MessageType, Native, Orientation, ResponseType, Root,
    ShortcutManager, Widget,
};
use regex::Regex;

fn main() {
    let app = Application::builder()
        .application_id("page.codeberg.alexispurslane.kissg")
        .build();

    let (send, recv): (Sender<Event>, Receiver<Event>) = unbounded();
    app.connect_activate(move |app| {
        let window = MainWindow::new(&app);
        window.imp().new_page.connect_clicked(
            glib::clone!(@weak window, @strong send => move |_| {
                send.send_blocking(Event::Reset);
                glib::MainContext::default()
                .spawn_local(MainWindow::choose_template_file(send.clone()));
            }),
        );

        window
            .imp()
            .choose_template
            .connect_activated(glib::clone!(@strong send => move |_| {
                glib::MainContext::default()
                .spawn_local(MainWindow::choose_template_file(send.clone()));
            }));

        window
            .imp()
            .choose_content
            .connect_activated(glib::clone!(@strong send => move |_| {
                glib::MainContext::default()
                .spawn_local(MainWindow::choose_content_file(send.clone()));
            }));

        window
            .imp()
            .choose_save
            .connect_activated(glib::clone!(@strong send => move |_| {
                glib::MainContext::default()
                .spawn_local(MainWindow::choose_save_location(send.clone()));
            }));

        window
            .imp()
            .finish
            .connect_clicked(glib::clone!(@weak window => move |_| {
                window.template_and_save();
            }));
        window.show();

        glib::spawn_future_local(glib::clone!(@weak window, @strong recv => async move {
            while let Ok(event) = recv.recv().await {
                println!("{:?}", event);
                match event {
                    Event::Reset => window.new_page(),
                    Event::SaveLocation(file) => window.update_save_location(file),
                    Event::Template { path, content } => window.update_template_file(path, content),
                    Event::Contents { path, content } => window.update_content_file(path, content),
                }
            }
        }));
    });

    app.run();
}

#[derive(Debug)]
enum Event {
    SaveLocation(gio::File),
    Template { path: String, content: String },
    Contents { path: String, content: String },
    Reset,
}

glib::wrapper! {
    pub struct MainWindow(ObjectSubclass<imp::MainWindow>)
        @extends ApplicationWindow, Window, Widget,
        @implements ActionGroup, ActionMap, Accessible, Buildable,
    ConstraintTarget, Native, Root, ShortcutManager;
}

mod imp {
    use std::collections::HashMap;

    use adw::PreferencesGroup;

    use super::*;
    #[derive(Default)]
    pub struct WindowState {
        pub template: Option<String>,
        pub content: Option<String>,
        pub save_location: Option<adw::gio::File>,
        pub variables: HashMap<String, EntryRow>,
    }

    #[derive(CompositeTemplate, Default)]
    #[template(file = "main-window.ui")]
    pub struct MainWindow {
        #[template_child(id = "new_page")]
        pub new_page: TemplateChild<Button>,

        #[template_child(id = "choose_save")]
        pub choose_save: TemplateChild<ActionRow>,

        #[template_child(id = "choose_template")]
        pub choose_template: TemplateChild<ActionRow>,

        #[template_child(id = "choose_content")]
        pub choose_content: TemplateChild<ActionRow>,

        #[template_child(id = "finish")]
        pub finish: TemplateChild<Button>,

        #[template_child(id = "page_content")]
        pub page_content_group: TemplateChild<PreferencesGroup>,

        pub state: Rc<RefCell<WindowState>>,
    }

    #[object_subclass]
    impl ObjectSubclass for MainWindow {
        const NAME: &'static str = "MainWindow";

        type Type = super::MainWindow;
        type ParentType = ApplicationWindow;

        fn class_init(klass: &mut Self::Class) {
            Self::bind_template(klass);
        }

        fn instance_init(obj: &InitializingObject<Self>) {
            obj.init_template();
        }
    }

    impl ObjectImpl for MainWindow {
        fn constructed(&self) {
            self.parent_constructed();
        }
    }

    impl WidgetImpl for MainWindow {}
    impl WindowImpl for MainWindow {}
    impl ApplicationWindowImpl for MainWindow {}
    impl AdwApplicationWindowImpl for MainWindow {}
}

impl MainWindow {
    pub fn new(app: &Application) -> Self {
        let klass: Self = Object::builder().property("application", app).build();

        klass
    }

    fn template_and_save(&self) {
        {
            let s = &*self.imp().state.borrow();
            if s.template.is_none() || s.content.is_none() || s.save_location.is_none() {
                Self::wizard_error_dialog();
                return;
            }

            // FIXME: This clones the string for ever template variable substituted
            let content = s.template.clone().unwrap();
            let content = s.variables.iter().fold(content, |acc, (k, v)| {
                acc.replace(&format!("{{{{{}}}}}", k), &v.text().to_string())
            });

            let res = content.replace("{{content}}", s.content.as_ref().unwrap());

            s.save_location
                .clone()
                .unwrap()
                .replace_contents(
                    res.as_bytes(),
                    None,
                    true,
                    adw::gio::FileCreateFlags::REPLACE_DESTINATION,
                    None::<&adw::gio::Cancellable>,
                )
                .unwrap();
        }

        let success_dialog = gtk4::MessageDialog::builder()
            .text("Finished publishing!")
            .secondary_text("Would you like to view the results?")
            .buttons(ButtonsType::YesNo)
            .build();
        let state = self.imp().state.clone();

        glib::MainContext::default().spawn_local(glib::clone!(@strong state => async move {
            match success_dialog.run_future().await {
                ResponseType::Yes => {
                    let dialog = gtk4::AppChooserDialog::new(
                        None::<&gtk4::Window>,
                        DialogFlags::empty(),
                        state.borrow().save_location.as_ref().expect("Save location undefined after publishing! This should never happen."),
                    );
                    match dialog.run_future().await {
                        ResponseType::Ok => {
                            dialog.app_info().unwrap().launch(
                                &[state.borrow().save_location.clone().unwrap()],
                                None::<&adw::gio::AppLaunchContext>,
                            ).unwrap();
                        }
                        _ => {}
                    }
                    dialog.close();
                },
                _ => {}
            }

            success_dialog.close();
        }));
    }

    fn file_chooser_dialog(
        title: &'static str,
        action: gtk4::FileChooserAction,
        button: &'static str,
        filters: &[&'static str],
    ) -> FileChooserNative {
        let filter = FileFilter::new();
        for s in filters {
            if s.contains("/") {
                // regular pattenrs can't contain slashes because
                // file names can't contain slashes because
                // they're path separators, so it must be a mime
                // type
                filter.add_mime_type(*s);
            } else {
                filter.add_pattern(*s);
            }
        }
        let file_chooser_dialog = FileChooserNative::builder()
            .title(title)
            .modal(true)
            .action(action)
            .filter(&filter)
            .accept_label(button)
            .cancel_label("Cancel")
            .build();
        file_chooser_dialog
    }

    fn error_dialog(title: String, message: String) {
        let error_dialog = gtk4::MessageDialog::builder()
            .message_type(MessageType::Error)
            .buttons(ButtonsType::Close)
            .text(title)
            .secondary_use_markup(true)
            .secondary_text(message)
            .build();
        error_dialog.run_async(move |error_dialog, _| {
            error_dialog.close();
        });
    }

    fn wizard_error_dialog() {
        Self::error_dialog(
            "⚠️ Page Creation Wizard Error".to_string(),
            "Cannot create a new page without all three of:\n\n1. a template\n2. page contents, and\n3. a destination HTML file to write the result to.".to_string()
        );
    }

    fn new_page(&self) {
        let sm = &mut *self.imp().state.borrow_mut();
        sm.template = None;
        self.imp().choose_template.set_subtitle("");
        sm.content = None;
        self.imp().choose_content.set_subtitle("");
        sm.save_location = None;
        self.imp().choose_save.set_subtitle("");
    }

    fn update_template_file(&self, path: String, template: String) {
        /// Create hashmap between template variable names and
        /// GUI entry rows to represent bindings for the template
        /// variables in this template, and add the entry rows to
        /// the perferences group
        fn inner(group: PreferencesGroup, template: &String) -> Option<HashMap<String, EntryRow>> {
            let re = Regex::new(r"\{\{([a-z-_]+)\}\}").ok()?;
            let mut variables = HashMap::new();
            for capture in re.captures_iter(&template) {
                let template_variable = capture.get(1)?.as_str().to_owned();
                if template_variable != "content" && !variables.contains_key(&template_variable) {
                    let mut chars = template_variable.chars();
                    let capitalized = format!(
                        "{}{}",
                        chars.nth(0).unwrap().to_uppercase(),
                        chars.collect::<String>()
                    );
                    println!("{}", capitalized);

                    let er = EntryRow::builder().title(capitalized).build();
                    variables.insert(template_variable, er.clone());
                    group.add(&er);
                }
            }
            Some(variables)
        }

        // Remove old template variable entry rows from the pref group
        for (_, er) in self.imp().state.borrow().variables.iter() {
            self.imp().page_content_group.remove(er);
        }

        // Add the template variable entry rows to the pref group
        // and add the hashmap for reading them later to the
        // window state.
        if let Some(vars) = inner(self.imp().page_content_group.clone(), &template) {
            self.imp().state.borrow_mut().variables = vars;
        } else {
            Self::error_dialog(
                "Templating Error".to_string(),
                "There was an error parsing the template you provided, please try again."
                    .to_string(),
            );
        }
        // Set the template content (avoid copying by casting gstring_content back)
        self.imp().state.borrow_mut().template = Some(template);

        // Make the subtitle in the template choosing action row the path
        // of the currently chosen template
        self.imp().choose_template.set_subtitle(&path);
    }

    // TODO: so much duplicate code, need to fix
    async fn choose_template_file(send: Sender<Event>) -> Option<()> {
        let file_chooser_dialog = Self::file_chooser_dialog(
            "Choose HTML template file",
            gtk4::FileChooserAction::Open,
            "Choose Template File",
            &["TEMPLATE-*", "text/html"],
        );

        let res = match file_chooser_dialog.run_future().await {
            ResponseType::Accept => {
                let file = file_chooser_dialog.file().unwrap();
                let (contents, _idk_what) = file.load_contents_future().await.unwrap();

                send.send(Event::Template {
                    content: std::str::from_utf8(&contents).unwrap().to_owned(),
                    path: file.path().unwrap().to_str().unwrap().to_owned(),
                })
                .await;
                Some(())
            }
            _ => None,
        };
        res
    }

    fn update_content_file(&self, path: String, content: String) {
        self.imp().state.borrow_mut().content = Some(content);
        self.imp().choose_content.set_subtitle(&path);
    }

    async fn choose_content_file(send: Sender<Event>) -> Option<()> {
        let file_chooser_dialog = Self::file_chooser_dialog(
            "Choose HTML content file",
            gtk4::FileChooserAction::Open,
            "Choose Content File",
            &["text/html"],
        );
        let res = match file_chooser_dialog.run_future().await {
            ResponseType::Accept => {
                let file = &file_chooser_dialog.file().unwrap();
                let (contents, _idk_what) = file.load_contents_future().await.unwrap();

                send.send(Event::Contents {
                    content: std::str::from_utf8(&contents).unwrap().to_owned(),
                    path: file.path().unwrap().to_str().unwrap().to_owned(),
                })
                .await;

                Some(())
            }
            _ => None,
        };
        res
    }

    fn update_save_location(&self, file: gio::File) {
        self.imp()
            .choose_save
            .set_subtitle(file.path().unwrap().to_str().unwrap());
        self.imp().state.borrow_mut().save_location = Some(file);
    }

    async fn choose_save_location(send: Sender<Event>) -> Option<()> {
        let file_chooser_dialog = Self::file_chooser_dialog(
            "Choose save location",
            gtk4::FileChooserAction::Save,
            "Save",
            &["text/html"],
        );

        let res = match file_chooser_dialog.run_future().await {
            ResponseType::Accept => {
                send.send(Event::SaveLocation(file_chooser_dialog.file().unwrap()))
                    .await;
                Some(())
            }
            _ => None,
        };
        res
    }
}
