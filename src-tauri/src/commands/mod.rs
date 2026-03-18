pub mod batch_separation;
pub mod bootstrap;
pub mod cdg;
pub mod error;
pub mod import;
pub mod library_setup;
pub mod lyrics;
pub mod maintenance;
pub mod playback;
pub mod separation;
pub mod settings;

pub use bootstrap::get_model_bootstrap_status;
pub use cdg::get_cdg_frame;
pub use error::{CommandError, CommandResult, ErrorCode, FallbackAction};
pub use import::{delete_songs, get_library, import_songs, search_library};
pub use lyrics::{fetch_lyrics, set_lyrics_offset};
pub use playback::{
    get_playback_state, load_stems, pause, play, seek, set_stem_volume, set_volume,
};
pub use separation::{get_separation_status, separate, upgrade_to_four_stem};
pub use settings::{get_settings, set_stem_mode};
