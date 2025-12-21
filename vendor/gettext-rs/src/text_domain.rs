//! A builder for gettext configuration.

use locale_config::{LanguageRange, Locale};

use std::env;
use std::error;
use std::fmt;
use std::fs;
use std::path::PathBuf;

use super::{bind_textdomain_codeset, bindtextdomain, setlocale, textdomain, LocaleCategory};

/// Errors that might come up after running the builder.
#[derive(Debug)]
pub enum TextDomainError {
    /// The locale is malformed.
    InvalidLocale(String),
    /// The translation for the requested language could not be found or the search path is empty.
    TranslationNotFound(String),
    /// The call to `textdomain()` failed.
    TextDomainCallFailed(std::io::Error),
    /// The call to `bindtextdomain()` failed.
    BindTextDomainCallFailed(std::io::Error),
    /// The call to `bind_textdomain_codeset()` failed.
    BindTextDomainCodesetCallFailed(std::io::Error),
}

impl fmt::Display for TextDomainError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        use TextDomainError::*;

        match self {
            InvalidLocale(locale) => write!(f, r#"Locale "{}" is invalid."#, locale),
            TranslationNotFound(language) => {
                write!(f, "Translations not found for language {}.", language)
            }
            TextDomainCallFailed(inner) => write!(f, "The call to textdomain() failed: {}", inner),
            BindTextDomainCallFailed(inner) => {
                write!(f, "The call to bindtextdomain() failed: {}", inner)
            }
            BindTextDomainCodesetCallFailed(inner) => {
                write!(f, "The call to bind_textdomain_codeset() failed: {}", inner)
            }
        }
    }
}

impl error::Error for TextDomainError {
    fn source(&self) -> Option<&(dyn error::Error + 'static)> {
        use TextDomainError::*;

        match self {
            InvalidLocale(_) => None,
            TranslationNotFound(_) => None,
            TextDomainCallFailed(inner) => Some(inner),
            BindTextDomainCallFailed(inner) => Some(inner),
            BindTextDomainCodesetCallFailed(inner) => Some(inner),
        }
    }
}

/// A builder to configure gettext.
///
/// It searches translations in the system data paths and optionally in the user-specified paths,
/// and binds them to the given domain. `TextDomain` takes care of calling [`setlocale`],
/// [`bindtextdomain`], [`bind_textdomain_codeset`], and [`textdomain`] for you.
///
/// # Defaults
///
/// - [`bind_textdomain_codeset`] is called by default to set UTF-8. You can use [`codeset`] to
/// override this, but please bear in mind that [other functions in this crate require
/// UTF-8](./index.html#utf-8-is-required).
/// - Current user's locale is selected by default. You can override this behaviour by calling
/// [`locale`].
/// - [`LocaleCategory::LcMessages`] is used when calling [`setlocale`]. Use [`locale_category`]
/// to override.
/// - System data paths are searched by default (see below for details). Use
/// [`skip_system_data_paths`] to limit the search to user-provided paths.
///
/// # Text domain path binding
///
/// A translation file for the text domain is searched in the following paths (in order):
///
/// 1. Paths added using the [`prepend`] function.
/// 1. Paths from the `XDG_DATA_DIRS` environment variable, except if the function
/// [`skip_system_data_paths`] was invoked. If `XDG_DATA_DIRS` is not set, or is empty, the default
/// of "/usr/local/share/:/usr/share/" is used.
/// 1. Paths added using the [`push`] function.
///
/// For each `path` in the search paths, the following subdirectories are scanned:
/// `path/locale/lang*/LC_MESSAGES` (where `lang` is the language part of the selected locale).
/// The first `path` containing a file matching `domainname.mo` is used for the call to
/// [`bindtextdomain`].
///
/// # Examples
///
/// Basic usage:
///
/// ```no_run
/// use gettextrs::TextDomain;
///
/// # fn main() -> Result<(), Box<dyn std::error::Error>> {
/// TextDomain::new("my_textdomain").init()?;
/// # Ok(())
/// # }
/// ```
///
/// Use the translation in current language under the `target` directory if available, otherwise
/// search system defined paths:
///
/// ```no_run
/// use gettextrs::TextDomain;
///
/// # fn main() -> Result<(), Box<dyn std::error::Error>> {
/// TextDomain::new("my_textdomain")
///            .prepend("target")
///            .init()?;
/// # Ok(())
/// # }
/// ```
///
/// Scan the `target` directory only, force locale to `fr_FR` and handle errors:
///
/// ```no_run
/// use gettextrs::{TextDomain, TextDomainError};
///
/// let init_msg = match TextDomain::new("my_textdomain")
///     .skip_system_data_paths()
///     .push("target")
///     .locale("fr_FR")
///     .init()
/// {
///     Ok(locale) => {
///         format!("translation found, `setlocale` returned {:?}", locale)
///     }
///     Err(error) => {
///         format!("an error occurred: {}", error)
///     }
/// };
/// println!("Textdomain init result: {}", init_msg);
/// ```
///
/// [`setlocale`]: fn.setlocale.html
/// [`bindtextdomain`]: fn.bindtextdomain.html
/// [`bind_textdomain_codeset`]: fn.bind_textdomain_codeset.html
/// [`textdomain`]: fn.textdomain.html
/// [`LocaleCategory::LcMessages`]: enum.LocaleCategory.html#variant.LcMessages
/// [`locale`]: struct.TextDomain.html#method.locale
/// [`locale_category`]: struct.TextDomain.html#method.locale_category
/// [`codeset`]: struct.TextDomain.html#method.codeset
/// [`skip_system_data_paths`]: struct.TextDomain.html#method.skip_system_data_paths
/// [`prepend`]: struct.TextDomain.html#method.prepend
/// [`push`]: struct.TextDomain.html#method.push
pub struct TextDomain {
    domainname: String,
    locale: Option<String>,
    locale_category: LocaleCategory,
    codeset: String,
    pre_paths: Vec<PathBuf>,
    post_paths: Vec<PathBuf>,
    skip_system_data_paths: bool,
}

impl TextDomain {
    /// Creates a new instance of `TextDomain` for the specified `domainname`.
    ///
    /// # Examples
    ///
    /// ```no_run
    /// use gettextrs::TextDomain;
    ///
    /// let text_domain = TextDomain::new("my_textdomain");
    /// ```
    pub fn new<S: Into<String>>(domainname: S) -> TextDomain {
        TextDomain {
            domainname: domainname.into(),
            locale: None,
            locale_category: LocaleCategory::LcMessages,
            codeset: "UTF-8".to_string(),
            pre_paths: vec![],
            post_paths: vec![],
            skip_system_data_paths: false,
        }
    }

    /// Override the `locale` for the `TextDomain`. Default is to use current locale.
    ///
    /// # Examples
    ///
    /// ```no_run
    /// use gettextrs::TextDomain;
    ///
    /// let text_domain = TextDomain::new("my_textdomain")
    ///                              .locale("fr_FR.UTF-8");
    /// ```
    pub fn locale(mut self, locale: &str) -> Self {
        self.locale = Some(locale.to_owned());
        self
    }

    /// Override the `locale_category`. Default is [`LocaleCategory::LcMessages`].
    ///
    /// # Examples
    ///
    /// ```no_run
    /// use gettextrs::{LocaleCategory, TextDomain};
    ///
    /// let text_domain = TextDomain::new("my_textdomain")
    ///                              .locale_category(LocaleCategory::LcAll);
    /// ```
    ///
    /// [`LocaleCategory::LcMessages`]: enum.LocaleCategory.html#variant.LcMessages
    pub fn locale_category(mut self, locale_category: LocaleCategory) -> Self {
        self.locale_category = locale_category;
        self
    }

    /// Define the `codeset` that will be used for calling [`bind_textdomain_codeset`]. The default
    /// is "UTF-8".
    ///
    /// **Warning:** [other functions in this crate require UTF-8](./index.html#utf-8-is-required).
    ///
    /// # Examples
    ///
    /// ```no_run
    /// use gettextrs::TextDomain;
    ///
    /// let text_domain = TextDomain::new("my_textdomain")
    ///                              .codeset("KOI8-R");
    /// ```
    ///
    /// [`bind_textdomain_codeset`]: fn.bind_textdomain_codeset.html
    pub fn codeset<S: Into<String>>(mut self, codeset: S) -> Self {
        self.codeset = codeset.into();
        self
    }

    /// Prepend the given `path` to the search paths.
    ///
    /// # Examples
    ///
    /// ```no_run
    /// use gettextrs::TextDomain;
    ///
    /// let text_domain = TextDomain::new("my_textdomain")
    ///                              .prepend("~/.local/share");
    /// ```
    pub fn prepend<P: Into<PathBuf>>(mut self, path: P) -> Self {
        self.pre_paths.push(path.into());
        self
    }

    /// Push the given `path` to the end of the search paths.
    ///
    /// # Examples
    ///
    /// ```no_run
    /// use gettextrs::TextDomain;
    ///
    /// let text_domain = TextDomain::new("my_textdomain")
    ///                              .push("test");
    /// ```
    pub fn push<P: Into<PathBuf>>(mut self, path: P) -> Self {
        self.post_paths.push(path.into());
        self
    }

    /// Don't search for translations in the system data paths.
    ///
    /// # Examples
    ///
    /// ```no_run
    /// use gettextrs::TextDomain;
    ///
    /// let text_domain = TextDomain::new("my_textdomain")
    ///                              .push("test")
    ///                              .skip_system_data_paths();
    /// ```
    pub fn skip_system_data_paths(mut self) -> Self {
        self.skip_system_data_paths = true;
        self
    }

    /// Search for translations in the search paths, initialize the locale, set up the text domain
    /// and ask gettext to convert messages to UTF-8.
    ///
    /// Returns an `Option` with the opaque string that describes the locale set (i.e. the result
    /// of [`setlocale`]) if:
    ///
    /// - a translation of the text domain in the requested language was found; and
    /// - the locale is valid.
    ///
    /// # Examples
    ///
    /// ```no_run
    /// use gettextrs::TextDomain;
    ///
    /// # fn main() -> Result<(), Box<dyn std::error::Error>> {
    /// TextDomain::new("my_textdomain").init()?;
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// [`TextDomainError`]: enum.TextDomainError.html
    /// [`setlocale`]: fn.setlocale.html
    pub fn init(mut self) -> Result<Option<Vec<u8>>, TextDomainError> {
        let (req_locale, norm_locale) = match self.locale.take() {
            Some(req_locale) => {
                if req_locale == "C" || req_locale == "POSIX" {
                    return Ok(Some(req_locale.as_bytes().to_owned()));
                }
                match LanguageRange::new(&req_locale) {
                    Ok(lang_range) => (req_locale.clone(), lang_range.into()),
                    Err(_) => {
                        // try again as unix language tag
                        match LanguageRange::from_unix(&req_locale) {
                            Ok(lang_range) => (req_locale.clone(), lang_range.into()),
                            Err(_) => {
                                return Err(TextDomainError::InvalidLocale(req_locale.clone()));
                            }
                        }
                    }
                }
            }
            None => {
                // `setlocale` accepts an empty string for current locale
                ("".to_owned(), Locale::current())
            }
        };

        let lang = norm_locale.as_ref().splitn(2, "-").collect::<Vec<&str>>()[0].to_owned();

        let domainname = self.domainname;
        let locale_category = self.locale_category;
        let codeset = self.codeset;

        let mo_rel_path = PathBuf::from("LC_MESSAGES").join(&format!("{}.mo", &domainname));

        // Get paths from system data dirs if requested so
        let sys_data_paths_str = if !self.skip_system_data_paths {
            get_system_data_paths()
        } else {
            "".to_owned()
        };
        let sys_data_dirs_iter = env::split_paths(&sys_data_paths_str);

        // Chain search paths and search for the translation mo file
        self.pre_paths
            .into_iter()
            .chain(sys_data_dirs_iter)
            .chain(self.post_paths.into_iter())
            .find(|path| {
                let locale_path = path.join("locale");
                if !locale_path.is_dir() {
                    return false;
                }

                // path contains a `locale` directory
                // search for sub directories matching `lang*`
                // and see if we can find a translation file for the `textdomain`
                // under `path/locale/lang*/LC_MESSAGES/`
                if let Ok(entry_iter) = fs::read_dir(&locale_path) {
                    return entry_iter
                        .filter_map(|entry_res| entry_res.ok())
                        .filter(|entry| {
                            matches!(
                                entry.file_type().map(|ft| ft.is_dir() || ft.is_symlink()),
                                Ok(true)
                            )
                        })
                        .any(|entry| {
                            if let Some(entry_name) = entry.file_name().to_str() {
                                return entry_name.starts_with(&lang)
                                    && locale_path.join(entry_name).join(&mo_rel_path).exists();
                            }

                            false
                        });
                }

                false
            })
            .map_or(Err(TextDomainError::TranslationNotFound(lang)), |path| {
                let result = setlocale(locale_category, req_locale);
                bindtextdomain(domainname.clone(), path.join("locale"))
                    .map_err(TextDomainError::BindTextDomainCallFailed)?;
                bind_textdomain_codeset(domainname.clone(), codeset)
                    .map_err(TextDomainError::BindTextDomainCodesetCallFailed)?;
                textdomain(domainname).map_err(TextDomainError::TextDomainCallFailed)?;
                Ok(result)
            })
    }
}

fn get_system_data_paths() -> String {
    static DEFAULT: &str = "/usr/local/share/:/usr/share/";

    if let Ok(dirs) = env::var("XDG_DATA_DIRS") {
        if dirs.is_empty() {
            DEFAULT.to_owned()
        } else {
            dirs
        }
    } else {
        DEFAULT.to_owned()
    }
}

impl fmt::Debug for TextDomain {
    fn fmt(&self, fmt: &mut fmt::Formatter) -> fmt::Result {
        let mut debug_struct = fmt.debug_struct("TextDomain");
        debug_struct
            .field("domainname", &self.domainname)
            .field(
                "locale",
                &match self.locale.as_ref() {
                    Some(locale) => locale.to_owned(),
                    None => {
                        let cur_locale = Locale::current();
                        cur_locale.as_ref().to_owned()
                    }
                },
            )
            .field("locale_category", &self.locale_category)
            .field("codeset", &self.codeset)
            .field("pre_paths", &self.pre_paths);

        if !self.skip_system_data_paths {
            debug_struct.field("using system data paths", &get_system_data_paths());
        }

        debug_struct.field("post_paths", &self.post_paths).finish()
    }
}

#[cfg(test)]
mod tests {
    use super::{LocaleCategory, TextDomain, TextDomainError};

    #[test]
    fn errors() {
        match TextDomain::new("test").locale("(°_°)").init().err() {
            Some(TextDomainError::InvalidLocale(message)) => assert_eq!(message, "(°_°)"),
            _ => panic!(),
        };

        match TextDomain::new("0_0").locale("en_US").init().err() {
            Some(TextDomainError::TranslationNotFound(message)) => assert_eq!(message, "en"),
            _ => panic!(),
        };
    }

    #[test]
    fn attributes() {
        let text_domain = TextDomain::new("test");
        assert_eq!("test".to_owned(), text_domain.domainname);
        assert!(text_domain.locale.is_none());
        assert_eq!(LocaleCategory::LcMessages, text_domain.locale_category);
        assert_eq!(text_domain.codeset, "UTF-8");
        assert!(text_domain.pre_paths.is_empty());
        assert!(text_domain.post_paths.is_empty());
        assert!(!text_domain.skip_system_data_paths);

        let text_domain = text_domain.locale_category(LocaleCategory::LcAll);
        assert_eq!(LocaleCategory::LcAll, text_domain.locale_category);

        let text_domain = text_domain.codeset("ISO-8859-15");
        assert_eq!("ISO-8859-15", text_domain.codeset);

        let text_domain = text_domain.prepend("pre");
        assert!(!text_domain.pre_paths.is_empty());

        let text_domain = text_domain.push("post");
        assert!(!text_domain.post_paths.is_empty());

        let text_domain = text_domain.skip_system_data_paths();
        assert!(text_domain.skip_system_data_paths);

        let text_domain = TextDomain::new("test").locale("en_US");
        assert_eq!(Some("en_US".to_owned()), text_domain.locale);

        // accept locale, but fail to find translation
        match TextDomain::new("0_0").locale("en_US").init().err() {
            Some(TextDomainError::TranslationNotFound(message)) => assert_eq!(message, "en"),
            _ => panic!(),
        };
    }
}
