# Database Schema

This document summarizes the current SQLite schema as defined by `src-tauri/migrations/001_init.sql` through `src-tauri/migrations/007_song_instrumental.sql`.

## Tables

### `songs`

Created by `001_init.sql`.

| Column              | Type      | Notes                                                  |
| ------------------- | --------- | ------------------------------------------------------ |
| `hash`              | `TEXT`    | Primary key for the imported song                      |
| `file_path`         | `TEXT`    | Normalized path stored relative to the library root    |
| `audio_source_kind` | `TEXT`    | Original, remote original, or remote stems source kind |
| `instrumental`      | `INTEGER` | Whether the song is marked as an official instrumental |
| `title`             | `TEXT`    | Extracted track title                                  |
| `artist`            | `TEXT`    | Extracted artist                                       |
| `album`             | `TEXT`    | Extracted album                                        |
| `duration_ms`       | `INTEGER` | Track duration in milliseconds                         |
| `cover_art`         | `BLOB`    | Embedded artwork bytes when available                  |
| `imported_at`       | `INTEGER` | Import timestamp                                       |

### `stems`

Created by `002_stems.sql`, expanded by `005_individual_stem_paths.sql` and `006_stem_model_variant.sql`.

| Column          | Type      | Notes                                                 |
| --------------- | --------- | ----------------------------------------------------- |
| `song_hash`     | `TEXT`    | Primary key and foreign key to `songs(hash)`          |
| `vocals_path`   | `TEXT`    | Cached vocals stem path                               |
| `accomp_path`   | `TEXT`    | Cached accompaniment stem path                        |
| `separated_at`  | `INTEGER` | Separation timestamp                                  |
| `drums_path`    | `TEXT`    | Optional individual drums stem path                   |
| `bass_path`     | `TEXT`    | Optional individual bass stem path                    |
| `other_path`    | `TEXT`    | Optional individual other stem path                   |
| `model_variant` | `TEXT`    | Model variant used for separation, default `htdemucs` |

### `lyrics`

Created by `003_lyrics.sql`.

| Column       | Type      | Notes                                        |
| ------------ | --------- | -------------------------------------------- |
| `song_hash`  | `TEXT`    | Primary key and foreign key to `songs(hash)` |
| `lrc`        | `TEXT`    | Cached lyrics payload                        |
| `source`     | `TEXT`    | Source enum serialized as text               |
| `offset_ms`  | `INTEGER` | Per-song timing offset                       |
| `fetched_at` | `INTEGER` | Fetch timestamp                              |

### `library_meta`

Created by `004_portable_paths.sql`.

| Column  | Type   | Notes                            |
| ------- | ------ | -------------------------------- |
| `key`   | `TEXT` | Primary key for library metadata |
| `value` | `TEXT` | Stored metadata value            |

## Migration History

1. `001_init.sql` - creates the `songs` table.
2. `002_stems.sql` - creates the `stems` table for separated output paths.
3. `003_lyrics.sql` - creates the `lyrics` table for cached LRC and offsets.
4. `004_portable_paths.sql` - creates `library_meta` for library-level metadata and portability markers.
5. `005_audio_source_kind.sql` - adds `audio_source_kind` to `songs` so remote source types are explicit.
6. `005_individual_stem_paths.sql` - adds `drums_path`, `bass_path`, and `other_path` to `stems`.
7. `006_stem_model_variant.sql` - adds `model_variant` to `stems` so cache entries remain variant-aware.
8. `007_song_instrumental.sql` - adds `instrumental` to `songs` so official accompaniment tracks can be excluded from separation.
