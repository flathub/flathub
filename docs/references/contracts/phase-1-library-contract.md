# Phase 1 资料库契约

**Goal:** 固定 `Phase 1` 代码侧已经实现并准备交给 UI Agent 消费的资料库接口、数据结构与语义，减少联调期间的猜测成本。

**Current starting point:** 本契约对应分支 `codex/phase0-m0` 上 `feat: add metadata parsing module`、`feat: add songs sqlite cache`、`feat: add import songs command` 之后的状态。后续如字段或命令语义变更，必须先更新此文档再改 UI。

## Owner

- 代码 Agent：命令、SQLite、元数据解析、错误语义
- UI Agent：消费本契约，不单方面修改命令名、字段名、排序语义或错误结构

## Phase-by-phase task breakdown

### 已冻结能力

1. `import_songs(paths: Vec<String>, options?: ImportSongsOptions) -> ImportSongsResult`
2. `pick_import_paths(default_path?: String | null) -> Vec<String>`
3. `expand_import_paths(paths: Vec<String>) -> ExpandedImportPaths`
4. `get_library() -> Vec<Song>`
5. `search_library(query: String) -> Vec<Song>`
6. `set_songs_instrumental(song_ids: Vec<String>, instrumental: bool) -> Vec<Song>`
7. `extract_embedded_cover_art(song_ids: Vec<String>) -> ExtractEmbeddedCoverArtResult`
8. 本地元数据解析支持 MP3、FLAC、M4A
9. `songs` 表通过 `hash` 去重并执行 upsert

### 后续 Phase 依赖

1. `Phase 2` 播放功能会基于 `Song.hash` 作为稳定歌曲标识
2. `Phase 3` stems cache 会复用同一个文件 hash
3. `Phase 4` 歌词缓存也默认以 `song_hash` 建联

## Inputs / outputs / required dependencies

### Command: `import_songs`

**Input**

```json
{
  "paths": ["/absolute/or/relative/audio/path.mp3"],
  "options": {
    "explicit_cdg_by_audio_path": {
      "/imports/song.flac": "/imports/song.cdg"
    },
    "skip_cdg_for_audio_paths": ["/imports/song.mp3"]
  }
}
```

**Output**

```json
{
  "imported": [
    {
      "hash": "sha256 hex string",
      "file_path": "/absolute/path/to/file.mp3",
      "instrumental": false,
      "title": "optional string",
      "artist": "optional string",
      "album": "optional string",
      "duration_ms": 123456,
      "cover_art": [137, 80, 78, 71],
      "imported_at": 1760000000
    }
  ],
  "failed": [
    {
      "path": "/bad/path.mp3",
      "error": {
        "code": "media_read_failed",
        "message": "failed to open audio file at /bad/path.mp3",
        "retryable": false,
        "fallback": "reimport_song"
      }
    }
  ]
}
```

**Semantics**

1. 单个路径失败不会中断整个批次，结果会落入 `failed`
2. 成功导入的项目会立即写入 SQLite，并返回写入后的 `Song`
3. `hash` 基于文件原始字节的 SHA-256，不基于路径
4. `file_path` 在返回前会被 canonicalize 为绝对路径
5. 若标签中没有标题，后端会回退到文件名 stem
6. 单个失败项的 `error` 已是结构化 `CommandError`，字段定义见 [phase-5-error-contract.md](./phase-5-error-contract.md)
7. 若用户只选择音频文件，而磁盘上存在同名 `.cdg` sidecar，后端会自动按 CD+G 成对导入
8. 若用户显式选择 `.cdg` 文件且前端已完成歧义消解，`options.explicit_cdg_by_audio_path` 会指定哪首音频应与该 `.cdg` 配对
9. `options.skip_cdg_for_audio_paths` 用于阻止同一 stem 的其他音频因同名 `.cdg` 被隐式配对

### Command: `expand_import_paths`

**Input**

```json
{
  "paths": ["/absolute/or/relative/folder/or/file"]
}
```

**Output**

```json
{
  "paths": ["/music/library/track-a.mp3", "/music/library/nested/track-b.flac"],
  "song_count": 2
}
```

**Semantics**

1. 该命令用于导入前预扫描，不写数据库，不复制媒体
2. 输入既可以是文件也可以是文件夹；文件夹会递归展开
3. 递归深度上限固定为 `3` 层，覆盖常见 `artist/album/song` 目录，同时避免大型目录或网络挂载把 UI 卡住
4. 输出 `paths` 只包含支持导入的媒体/歌词相关文件，且会去重并排序
5. `song_count` 表示确认弹窗中应展示的歌曲数量；当前按可导入文件数统计
6. 该命令的输出可直接作为后续 `import_songs` 的输入

### Command: `pick_import_paths`

**Input**

```json
{
  "default_path": "/Users/example/Music"
}
```

**Output**

```json
["/Users/example/Music/library", "/Users/example/Downloads/song.mp3"]
```

**Semantics**

1. 该命令负责打开导入选择器，本身不做扫描、不写数据库
2. macOS 上允许同一个原生面板同时选择文件和文件夹，且支持多选
3. 前端会把返回结果继续交给 `expand_import_paths` 做递归展开和数量确认
4. 非 macOS 当前不依赖此命令；前端保留直接文件选择回退

### Command: `get_library`

**Output:** `Vec<Song>`

**Semantics**

1. 排序为 `imported_at DESC, title COLLATE NOCASE ASC, hash ASC`
2. 当前不分页
3. 当前不做软删除过滤，因为还没有删除能力
4. 顶层命令失败时返回 `CommandError`，而不是自由文本字符串

### Command: `search_library`

**Input**

```json
{
  "query": "muse"
}
```

**Output:** `Vec<Song>`

**Semantics**

1. 大小写不敏感
2. 匹配范围：`title`、`artist`、`album`、`file_path`
3. 排序规则与 `get_library` 相同
4. 顶层命令失败时返回 `CommandError`

### Command: `extract_embedded_cover_art`

**Input**

```json
{
  "song_ids": ["sha256 song hash"]
}
```

**Output**

```json
{
  "updated_songs": [
    {
      "hash": "sha256 hex string",
      "file_path": "media/song.mp3",
      "title": "optional string",
      "artist": "optional string",
      "album": "optional string",
      "duration_ms": 123456,
      "cover_art": [137, 80, 78, 71],
      "imported_at": 1760000000
    }
  ],
  "failed": [
    {
      "song_id": "sha256 song hash",
      "error": {
        "code": "media_read_failed",
        "message": "song hash does not contain embedded cover art",
        "retryable": false,
        "fallback": "keep_current_state"
      }
    }
  ]
}
```

**Semantics**

1. 批量按顺序处理，单首失败不会中断其他歌曲
2. 成功项会覆盖 `songs.cover_art`，并返回更新后的完整 `Song`
3. 普通音频与 `paired` CDG 从磁盘音频文件读取封面；`ZIP+G` 从 ZIP 内音频字节读取封面
4. 若文件没有内嵌封面，当前数据库里的 `cover_art` 保持不变，并在 `failed` 中返回结构化错误
5. 顶层命令只在数据库不可用等整体失败时返回 `CommandError`

### Command: `set_songs_instrumental`

**Input**

```json
{
  "song_ids": ["sha256 song hash"],
  "instrumental": true
}
```

**Output:** `Vec<Song>`

**Semantics**

1. 批量按请求顺序更新 `songs.instrumental`
2. 返回值包含每首更新后的完整 `Song`
3. `instrumental = true` 表示该歌曲被视为官方伴奏，不参与 AI 分离
4. `Media+G` 歌曲当前不会由前端发起该命令，但后端字段本身不额外限制素材类型
5. 若任一 `song_id` 不存在，命令返回顶层 `CommandError`

### Shared type: `Song`

| Field          | Type              | Notes                                          |
| -------------- | ----------------- | ---------------------------------------------- |
| `hash`         | `String`          | 全局稳定主键                                   |
| `file_path`    | `String`          | canonicalized 绝对路径                         |
| `instrumental` | `bool`            | 是否标记为官方伴奏；`true` 时不参与 AI 分离    |
| `title`        | `Option<String>`  | 可能为空                                       |
| `artist`       | `Option<String>`  | 可能为空                                       |
| `album`        | `Option<String>`  | 可能为空                                       |
| `duration_ms`  | `i64`             | 当前来自音频元数据                             |
| `cover_art`    | `Option<Vec<u8>>` | 原始图片字节，前端需自行转 data URL/object URL |
| `imported_at`  | `i64`             | Unix timestamp seconds                         |

### Shared type: `ImportFailure`

| Field   | Type           | Notes                                                                           |
| ------- | -------------- | ------------------------------------------------------------------------------- |
| `path`  | `String`       | 原始输入路径                                                                    |
| `error` | `CommandError` | 结构化错误，字段定义见 [phase-5-error-contract.md](./phase-5-error-contract.md) |

### Shared type: `ImportSongsOptions`

| Field                        | Type                    | Notes                                          |
| ---------------------------- | ----------------------- | ---------------------------------------------- |
| `explicit_cdg_by_audio_path` | `Record<String,String>` | 指定某首音频应使用哪一个显式选择的 `.cdg` 文件 |
| `skip_cdg_for_audio_paths`   | `Vec<String>`           | 阻止这些音频在本次导入中被 `.cdg` 自动配对     |

### Shared type: `ExpandedImportPaths`

| Field        | Type          | Notes                            |
| ------------ | ------------- | -------------------------------- |
| `paths`      | `Vec<String>` | 递归展开、去重并排序后的导入路径 |
| `song_count` | `usize`       | 导入前确认弹窗使用的歌曲数量     |

### Shared type: `ExtractEmbeddedCoverArtResult`

| Field           | Type                                  | Notes                      |
| --------------- | ------------------------------------- | -------------------------- |
| `updated_songs` | `Vec<Song>`                           | 成功提取并写回封面的歌曲   |
| `failed`        | `Vec<ExtractEmbeddedCoverArtFailure>` | 逐首失败结果，允许部分成功 |

### Shared type: `ExtractEmbeddedCoverArtFailure`

| Field     | Type           | Notes                |
| --------- | -------------- | -------------------- |
| `song_id` | `String`       | 请求中的歌曲 hash    |
| `error`   | `CommandError` | 单首失败的结构化错误 |

### Required dependencies

1. Rust crate `lofty` 负责读标签和时长
2. Rust crate `rusqlite` 负责持久化
3. Rust crate `sha2` 负责生成稳定文件 hash
4. Tauri app setup 必须先完成 `AppState.database_path` 注入

## Verification commands

```bash
cd src-tauri
cargo test --test phase1_metadata --test phase1_cache --test phase1_import
cargo test
```

**Expected evidence**

1. 三个 Phase 1 integration tests 全部通过
2. `cache` 的 migration 单元测试通过
3. 无需运行 UI 也能验证导入、搜索、落库语义

## Pause-and-resume instructions

1. 接手前先读本文件，再读 [../internal/roadmap.md](../internal/roadmap.md)
2. 执行上面的验证命令，确认当前分支状态真实可用
3. 若后续需要修改字段或命令语义：
   - 先更新本契约
   - 再修改 Rust 实现
   - 最后通知 UI Agent 按新契约调整
4. 若工作中断，至少在提交信息或交接说明里标出：
   - 改了哪些命令
   - 是否改了 `Song` 字段
   - 哪些测试已跑，哪些没跑
