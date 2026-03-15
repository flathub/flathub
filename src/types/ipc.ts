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

export interface Song {
  hash: string;
  file_path: string;
  title: string | null;
  artist: string | null;
  album: string | null;
  duration_ms: number;
  cover_art: number[] | null;
  imported_at: number;
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

export interface LyricsMatch {
  song_hash: string;
  lrc_path: string;
}

export interface ImportLyricsResult {
  matched: LyricsMatch[];
  unmatched: string[];
}

// ─── Settings ───────────────────────────────────────────

export type StemMode = "two_stem" | "four_stem";

export interface AppSettings {
  stem_mode: StemMode;
}

// ─── Playback ────────────────────────────────────────────

export type StemName = "vocals" | "drums" | "bass" | "other";

export interface StemVolumes {
  vocals: number;
  drums: number;
  bass: number;
  other: number;
}

export interface PlaybackStateSnapshot {
  song_id: string | null;
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
}

export interface PlaybackEndedEvent {
  song_id: string;
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
  error: CommandError | null;
}

export interface SeparationProgressEvent {
  song_id: string;
  percent: number;
}

export interface SeparationCompleteEvent {
  song_id: string;
}

export interface SeparationErrorEvent {
  song_id: string;
  error: CommandError;
}

// ─── Lyrics ──────────────────────────────────────────────

export type LyricsSource = "lrc_lib" | "embedded" | "sidecar" | "manual";

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

// ─── Model Bootstrap ────────────────────────────────────

export type ModelBootstrapState =
  | "pending"
  | "downloading"
  | "ready"
  | "failed";

export interface ModelBootstrapStatusSnapshot {
  state: ModelBootstrapState;
  model_path: string;
  downloaded_bytes: number | null;
  total_bytes: number | null;
  error: CommandError | null;
}
