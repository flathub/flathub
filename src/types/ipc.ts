// All types mirror Rust struct serialization exactly.
// Struct fields: snake_case (no rename_all on structs).
// Enum variants: snake_case (via #[serde(rename_all = "snake_case")]).

// ─── Error ───────────────────────────────────────────────

export type ErrorCode =
  | "database_unavailable"
  | "media_read_failed"
  | "song_not_found"
  | "model_unavailable"
  | "audio_decode_failed"
  | "audio_output_unavailable"
  | "karaoke_not_ready"
  | "lyrics_not_ready"
  | "network_unavailable"
  | "invalid_playback_state"
  | "separation_failed"
  | "internal";

export type FallbackAction =
  | "retry"
  | "refresh_library"
  | "reimport_song"
  | "check_audio_output_device"
  | "stay_in_original_mode"
  | "show_empty_state"
  | "keep_current_state";

export interface CommandError {
  code: ErrorCode;
  message: string;
  retryable: boolean;
  fallback: FallbackAction;
}

// ─── Library ─────────────────────────────────────────────

export type CoverArtBytes = number[] | Uint8Array | ArrayBuffer | null;
export type LibraryKind = "local" | "remote";
export type RemoteLibraryProvider = "google_drive" | "dropbox" | "webdav";

export interface WebDavRemoteAuthPayload {
  type: "webdav";
  server_url: string;
  username: string;
  password: string;
  root_path: string | null;
}

export type RemoteAuthPayload = WebDavRemoteAuthPayload | null;

export interface RemoteAuthStart {
  session_id: string;
  provider: RemoteLibraryProvider;
  authorization_url: string | null;
  expires_at_ms: number | null;
}

export type RemoteAuthState = "pending" | "ready" | "failed";

export interface RemoteAuthStatus {
  session_id: string;
  provider: RemoteLibraryProvider;
  state: RemoteAuthState;
  remote_root_locator: string | null;
  display_name: string | null;
  error: CommandError | null;
}

export interface RemoteLibraryCandidate {
  provider: RemoteLibraryProvider;
  remote_root_locator: string;
  remote_path_display: string;
  display_name: string;
  account_id: string | null;
}

export interface LocalLibraryRegistration {
  id: string;
  kind: "local";
  display_name: string;
  root_path: string;
}

export interface RemoteLibraryRegistration {
  id: string;
  kind: "remote";
  display_name: string;
  provider: RemoteLibraryProvider;
  remote_root_locator: string;
  remote_path_display: string;
  account_id: string;
  connection_config: RemoteLibraryConnectionConfig | null;
  cached_db_path: string | null;
  remote_revision: string | null;
}

export type RemoteLibraryConnectionConfig =
  | {
      type: "google_drive";
      oauth_client_id: string;
    }
  | {
      type: "dropbox";
      app_key: string;
    }
  | {
      type: "webdav";
      server_url: string;
    };

export type RegisteredLibrary =
  | LocalLibraryRegistration
  | RemoteLibraryRegistration;

export interface LibraryRegistrySnapshot {
  active_library_id: string | null;
  libraries: RegisteredLibrary[];
}

export interface Song {
  hash: string;
  file_path: string | null;
  audio_source_kind: "original" | "original_remote" | "stems_remote";
  cdg_path: string | null;
  media_g_container: "paired" | "zip" | null;
  instrumental: boolean;
  title: string | null;
  artist: string | null;
  album: string | null;
  duration_ms: number;
  cover_art: CoverArtBytes;
  imported_at: number;
  original_ext: string | null;
}

export interface SongProperties {
  format: string;
  sample_rate: number | null;
  channels: number | null;
  bit_rate: number | null;
  file_size: number;
  duration_ms: number;
  hash: string;
}

export interface ImportFailure {
  path: string;
  error: CommandError;
}

export interface ImportSongsResult {
  imported: Song[];
  failed: ImportFailure[];
}

export interface ExpandedImportPaths {
  paths: string[];
  song_count: number;
}

export interface ImportSongsOptions {
  explicit_cdg_by_audio_path: Record<string, string>;
  skip_cdg_for_audio_paths?: string[];
}

export interface ImportCandidateDetails {
  path: string;
  format: string;
  bit_rate: number | null;
  file_size: number;
  duration_ms: number | null;
}

export interface DeleteSongsFailure {
  song_id: string;
  error: CommandError;
}

export interface DeleteSongsResult {
  deleted_song_ids: string[];
  failed: DeleteSongsFailure[];
}

export interface ExtractEmbeddedCoverArtFailure {
  song_id: string;
  error: CommandError;
}

export interface ExtractEmbeddedCoverArtResult {
  updated_songs: Song[];
  failed: ExtractEmbeddedCoverArtFailure[];
}

export interface LyricsMatch {
  song_id: string;
  lrc_path: string;
}

export interface ImportLyricsResult {
  matched: LyricsMatch[];
  unmatched: string[];
}

// ─── Settings ───────────────────────────────────────────

export type StemMode = "two_stem" | "four_stem";
export type ModelVariant = "htdemucs" | "htdemucs_ft";
export type ExecutionProvider = "cpu" | "xnnpack" | "directml";

export interface AppSettings {
  stem_mode: StemMode;
  model_variant: ModelVariant;
  language: string | null;
  hide_batch_separate: boolean;
  cover_art_backdrop: boolean;
  lyrics_font_step: number;
  execution_provider: ExecutionProvider;
  available_execution_providers: ExecutionProvider[];
}

export interface ModelStatusSnapshot {
  variant: string;
  downloaded: boolean;
  /** Managed file exists but SHA-256 does not match the pinned release. */
  legacy_install_present: boolean;
  file_size: number | null;
}

export type WindowShellChromeVariant = "desktop" | "mac";
export type WindowShellTier = "desktop" | "mac";

export interface WindowShellStateSnapshot {
  chrome_variant: WindowShellChromeVariant;
  tier: WindowShellTier;
  toolbar_height: number;
  traffic_light_inset_leading: number;
  sidebar_header_height: number;
  sidebar_width: number;
}

// ─── Playback ────────────────────────────────────────────

export type StemName = "vocals" | "drums" | "bass" | "other";
export type PlaybackTransportState = "idle" | "loading" | "playing";

export interface StemVolumes {
  vocals: number;
  drums: number;
  bass: number;
  other: number;
}

export interface PlaybackStateSnapshot {
  song_id: string | null;
  /** Backend transport lifecycle; pause is represented by `is_playing: false`. */
  state: PlaybackTransportState;
  is_playing: boolean;
  position_ms: number;
  duration_ms: number | null;
  volume: number;
  stem_volumes: StemVolumes;
  has_stems: boolean;
  stem_mode: StemMode | null;
}

export interface PlaybackPositionEvent {
  ms: number;
  snapshot: PlaybackStateSnapshot;
}

export interface PlaybackEndedEvent {
  song_id: string;
}

export interface AirPlayRoutePickerBounds {
  left: number;
  top: number;
  width: number;
  height: number;
}

export type AirPlayAudienceMode = "idle" | "lyrics" | "cdg";
export type AirPlayOutputPhase =
  | "idle"
  | "route_selected"
  | "buffering"
  | "playing"
  | "failed";

export interface AirPlayViewport {
  widthPx: number;
  heightPx: number;
  bottomInsetPx: number;
}

export interface AirPlayColor {
  red: number;
  green: number;
  blue: number;
  alpha: number;
}

export interface AudiencePresentationSpec {
  contentWidthRatio: number;
  contentMaxWidthPx: number;
  horizontalPaddingPx: number;
  verticalPaddingPx: number;
  lineGapPx: number;
  fontSizePx: number;
  lineHeightMultiple: number;
  activeScale: number;
  statusFontSizePx: number;
  activeGlowBlurPx: number;
  activeTextColor: AirPlayColor;
  pastTextColor: AirPlayColor;
  futureTextColor: AirPlayColor;
  plainTextColor: AirPlayColor;
  statusTextColor: AirPlayColor;
  activeGlowColor: AirPlayColor;
}

export interface AirPlayAudienceMessages {
  selectSong: string;
  loadingLyrics: string;
  noLyrics: string;
  addLyrics: string;
}

export interface AirPlayAudienceStatePayload {
  mode: AirPlayAudienceMode;
  songId: string | null;
  lines: LyricLine[];
  offsetMs: number;
  isLoading: boolean;
  lyricsFontStep: number;
  messages: AirPlayAudienceMessages;
  viewport: AirPlayViewport;
  presentationSpec: AudiencePresentationSpec;
}

export interface AirPlayOutputStateEvent {
  active: boolean;
  audioActive: boolean;
  routeName: string | null;
  mode: AirPlayAudienceMode;
  phase: AirPlayOutputPhase;
  detail: string | null;
  displayedPositionMs: number | null;
  streamGeneration: number;
  latencyMs: number | null;
}

// ─── Separation ──────────────────────────────────────────

export type SeparationState = "idle" | "running" | "completed" | "failed";

export interface SeparationStatusSnapshot {
  song_id: string;
  state: SeparationState;
  percent: number;
  cache_hit: boolean;
  vocals_path: string | null;
  accomp_path: string | null;
  drums_path: string | null;
  bass_path: string | null;
  other_path: string | null;
  model_variant: string | null;
  error: CommandError | null;
}

export interface SeparationProgressEvent {
  song_id: string;
  percent: number;
}

export interface SeparationCompleteEvent {
  song_id: string;
  status: SeparationStatusSnapshot;
}

export interface SeparationErrorEvent {
  song_id: string;
  error: CommandError;
}

export type UploadState = "idle" | "running" | "completed" | "failed";

export interface UploadStatusSnapshot {
  song_id: string;
  state: UploadState;
  percent: number;
  remote_library_id?: string | null;
  detail?: string | null;
  error: CommandError | null;
}

export interface UploadProgressEvent {
  song_id: string;
  percent: number;
  remote_library_id?: string | null;
  detail?: string | null;
}

export interface UploadCompleteEvent {
  song_id: string;
  remote_library_id?: string | null;
}

export interface UploadErrorEvent {
  song_id: string;
  remote_library_id?: string | null;
  error: CommandError;
}

// ─── Lyrics ──────────────────────────────────────────────

export type LyricsSource =
  | "lrc_lib"
  | "lrc_api"
  | "embedded"
  | "sidecar"
  | "manual";

export interface WordToken {
  time_ms: number;
  text: string;
}

export interface LyricLine {
  time_ms: number;
  text: string;
  words: WordToken[] | null;
}

export interface LyricsPayload {
  song_id: string;
  lines: LyricLine[];
  source: LyricsSource | null;
  offset_ms: number;
  raw_lrc: string;
}

// ─── Maintenance ────────────────────────────────────────

export interface DeleteStemsResult {
  deleted_count: number;
  freed_bytes: number;
}

export interface DowngradeResult {
  downgraded_count: number;
  freed_bytes: number;
}

export interface BatchSeparationProgress {
  total: number;
  completed: number;
  skipped: number;
  failed: number;
  current_song_id: string | null;
  current_percent: number;
}

// ─── Model Bootstrap ────────────────────────────────────

export type ModelBootstrapState =
  | "pending"
  | "downloading"
  | "outdated"
  | "ready"
  | "failed";

export interface ModelBootstrapStatusSnapshot {
  state: ModelBootstrapState;
  model_path: string;
  downloaded_bytes: number | null;
  total_bytes: number | null;
  error: CommandError | null;
}
