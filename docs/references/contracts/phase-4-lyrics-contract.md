# Phase 4 歌词契约

**Goal:** 固定 `Phase 4` 代码侧已经实现的歌词抓取、解析、缓存和 offset 持久化语义，保证 UI Agent 与后续接手者都基于同一套返回结构继续。

**Current starting point:** 本契约对应分支 `codex/phase0-m0` 上 LRCLIB client、LRC parser、抓取优先链、SQLite cache，以及 `fetch_lyrics / set_lyrics_offset` 命令接入之后的状态。

## Owner

- 代码 Agent：歌词来源选择、LRC 解析、SQLite cache、offset 持久化、命令错误语义
- UI Agent：消费命令返回值，不单方面修改命令名、字段名、source 语义或 miss 行为

## 已冻结能力

1. `fetch_lyrics(song_id: String) -> LyricsPayload`
2. `set_lyrics_offset(song_id: String, ms: i64) -> ()`
3. 抓取优先顺序固定为 `LRCLIB -> embedded -> sidecar .lrc`
4. SQLite `lyrics` 表按 `song_hash` 缓存原始 LRC 和 `offset_ms`
5. 对同一首歌重复调用 `fetch_lyrics` 时，优先命中 SQLite cache，不重复发起 HTTP 请求
6. `Phase 5` 起，歌词命令失败值统一为 `CommandError`，详见 [phase-5-error-contract.md](./phase-5-error-contract.md)

## Inputs / outputs / required dependencies

### Command: `fetch_lyrics`

**Input**

```json
{
  "song_id": "sha256 hash string"
}
```

**Output**

```json
{
  "song_id": "sha256 hash string",
  "lines": [
    {
      "time_ms": 35660,
      "text": "Look at the stars"
    }
  ],
  "source": "lrc_lib",
  "offset_ms": 0
}
```

**Miss output**

```json
{
  "song_id": "sha256 hash string",
  "lines": [],
  "source": null,
  "offset_ms": 0
}
```

**Semantics**

1. `song_id` 对应 `songs.hash`
2. 后端会先检查 SQLite `lyrics` cache；命中后直接解析缓存的 LRC
3. cache miss 时，后端按固定顺序尝试：
   - LRCLIB `GET /api/get`
   - 音频文件内嵌歌词标签
   - 同名 sidecar `.lrc`
4. 一旦抓到歌词，后端会先解析成 `Vec<LyricLine>`，再把原始 LRC、来源和 `offset_ms = 0` 写入 SQLite
5. 如果所有来源都 miss，命令仍然成功返回；只是 `lines = []`、`source = null`
6. 如果歌曲不存在、HTTP 请求失败、文件读取失败或 LRC 解析失败，命令返回 `CommandError`

### Command: `set_lyrics_offset`

**Input**

```json
{
  "song_id": "sha256 hash string",
  "ms": 500
}
```

**Semantics**

1. `ms` 为该歌曲的用户手动 timing offset，单位毫秒，可正可负
2. 只有在该歌曲已经存在缓存歌词时，命令才会成功
3. 如果歌曲存在但还没有缓存歌词，命令返回 `CommandError`
4. 该命令只更新 SQLite 中的 `offset_ms`，不会重抓歌词

### Shared type: `LyricsPayload`

| Field       | Type                                       | Notes                        |
| ----------- | ------------------------------------------ | ---------------------------- |
| `song_id`   | `String`                                   | 对应 `songs.hash`            |
| `lines`     | `Vec<LyricLine>`                           | 已按 `time_ms` 升序排序      |
| `source`    | `Option<"lrc_lib"\|"embedded"\|"sidecar">` | 无命中时为 `null`            |
| `offset_ms` | `i64`                                      | 当前已持久化的 timing offset |

### Shared type: `LyricLine`

| Field     | Type     | Notes                    |
| --------- | -------- | ------------------------ |
| `time_ms` | `u64`    | 行起始时间，单位毫秒     |
| `text`    | `String` | 当前时间戳对应显示的文本 |

### Shared error type: `CommandError`

歌词命令统一返回结构化错误，字段定义与错误码含义见 [phase-5-error-contract.md](./phase-5-error-contract.md)。

## Cache semantics

1. SQLite `lyrics` 表字段固定为：
   - `song_hash`
   - `lrc`
   - `source`
   - `offset_ms`
   - `fetched_at`
2. 当前不会为 miss 结果写入空缓存行；只有真实命中的歌词才会落库
3. `source` 序列化值固定为：
   - `lrc_lib`
   - `embedded`
   - `sidecar`

## Required dependencies

1. `reqwest` 负责 LRCLIB HTTP 请求
2. `lofty` 负责读取内嵌歌词标签
3. `rusqlite` 负责缓存和 offset 持久化
4. `playback-position` 事件继续由 Phase 2 播放契约提供，歌词契约本身不新增事件

## Verification commands

```bash
cd src-tauri
cargo test --test phase4_lrclib --test phase4_parser --test phase4_fetch --test phase4_lyrics_cache --test phase4_commands
cargo test
cd ..
pnpm tauri build --debug --no-bundle --ci
```

**Expected evidence**

1. `phase4_lrclib`
2. `phase4_parser`
3. `phase4_fetch`
4. `phase4_lyrics_cache`
5. `phase4_commands`

以上测试全部通过，并且调试构建成功。

## Pause-and-resume instructions

1. 接手前先读本文件，再读 [../../exec-plans/active/2026-03-13-code-agent-plan.md](../../exec-plans/active/2026-03-13-code-agent-plan.md)
2. 先跑验证命令，确认歌词后端没有被后续改动打破
3. 如果要改命令名、字段名、抓取顺序、miss 语义或 `source` 枚举值：
   - 先更新本契约
   - 再改 Rust 实现
   - 最后通知 UI Agent
4. UI Agent 消费本契约时，默认流程应为：
   - `play(song_id)`
   - `fetch_lyrics(song_id)`
   - 监听 `playback-position`
   - 需要微调时调用 `set_lyrics_offset(song_id, ms)`
