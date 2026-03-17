# Phase 5 错误处理契约

**Goal:** 固定当前后端已经开始使用的结构化错误模型，给 UI Agent 一个稳定的错误码、fallback 提示和 retry 语义来源，而不是继续解析自由文本字符串。

**Current starting point:** 本契约对应分支 `codex/phase0-m0` 上播放命令、歌词命令和分离状态错误都已经切到 `CommandError` 结构之后的状态。

## Owner

- 代码 Agent：错误码定义、命令错误映射、fallback 语义
- UI Agent：根据 `code / retryable / fallback` 决定 toast、empty state 和恢复动作，不单方面改字段名或枚举值

## 已冻结能力

1. `play / pause / seek / set_volume / set_playback_mode / get_playback_state` 失败时返回 `CommandError`
2. `fetch_lyrics / set_lyrics_offset` 失败时返回 `CommandError`
3. `SeparationStatusSnapshot.error` 变为 `Option<CommandError>`
4. `separation-error` 事件 payload 的 `error` 字段变为 `CommandError`
5. `import_songs.failed[].error` 变为 `CommandError`
6. `get_library / search_library` 顶层命令失败时返回 `CommandError`

## Shared type: `CommandError`

```json
{
  "code": "karaoke_not_ready",
  "message": "song with hash song-a does not have cached stems",
  "retryable": true,
  "fallback": "stay_in_original_mode"
}
```

| Field       | Type             | Notes                                  |
| ----------- | ---------------- | -------------------------------------- |
| `code`      | `ErrorCode`      | 稳定错误码，UI 不应再解析 message 文本 |
| `message`   | `String`         | 仍保留给日志、调试和用户提示           |
| `retryable` | `bool`           | 当前动作是否值得展示“重试”             |
| `fallback`  | `FallbackAction` | UI 默认回退策略提示                    |

## Shared enum: `ErrorCode`

- `database_unavailable`
- `media_read_failed`
- `song_not_found`
- `model_unavailable`
- `audio_decode_failed`
- `audio_output_unavailable`
- `karaoke_not_ready`
- `lyrics_not_ready`
- `network_unavailable`
- `invalid_playback_state`
- `separation_failed`
- `internal`

## Shared enum: `FallbackAction`

- `retry`
- `refresh_library`
- `reimport_song`
- `check_audio_output_device`
- `stay_in_original_mode`
- `show_empty_state`
- `keep_current_state`

## Current mapping semantics

### Library / Import

1. 导入单个文件时无法打开、无法读元数据、无法 canonicalize 路径：
   - `code = media_read_failed`
   - `fallback = reimport_song`
2. 资料库相关的 SQLite 打开或查询失败：
   - `code = database_unavailable`
   - `fallback = retry`

### Playback

1. 找不到歌曲：
   - `code = song_not_found`
   - `fallback = refresh_library`
2. 音频解码失败或文件损坏：
   - `code = audio_decode_failed`
   - `fallback = reimport_song`
3. Karaoke 模式缺少 cached stems：
   - `code = karaoke_not_ready`
   - `fallback = stay_in_original_mode`
4. 没有默认输出设备或设备配置失败：
   - `code = audio_output_unavailable`
   - `fallback = check_audio_output_device`

### Lyrics

1. 歌词缓存缺失或 LRC 不可用：
   - `code = lyrics_not_ready`
   - `fallback = show_empty_state`
2. LRCLIB 请求失败：
   - `code = network_unavailable`
   - `fallback = retry`
3. 歌曲不存在：
   - `code = song_not_found`
   - `fallback = refresh_library`

### Separation

1. 分离 worker 失败：
   - `code = separation_failed`
   - `fallback = retry`
2. 分离输入歌曲已丢失：
   - `code = song_not_found`
   - `fallback = refresh_library`
3. 分离前解码失败：
   - `code = audio_decode_failed`
   - `fallback = reimport_song`
4. 运行时模型尚未下载完成、校验失败或 bootstrap 已失败：
   - `code = model_unavailable`
   - `fallback = retry`

## Important boundaries

1. 当前错误分类先在 command 边界完成，底层模块仍主要返回 `anyhow::Error`
2. 如果后续把底层模块也切到 typed domain errors，必须保持这里定义的 `ErrorCode` 和 `FallbackAction` 对外稳定

## Verification commands

```bash
cd src-tauri
cargo test --test phase5_errors --test phase3_status --test phase1_import
cargo test
cd ..
pnpm tauri build --debug --no-bundle --ci
```

**Expected evidence**

1. `phase5_errors` 证明错误分类稳定
2. `phase1_import` 证明导入失败项已携带结构化错误
3. `phase3_status` 证明分离状态已携带结构化错误
4. 全量测试和调试构建通过

## Pause-and-resume instructions

1. 接手前先读本文件，再读 [../../exec-plans/active/2026-03-13-code-agent-plan.md](../../exec-plans/active/2026-03-13-code-agent-plan.md)
2. 若后续新增错误码或 fallback：
   - 先更新本契约
   - 再改 Rust 实现和测试
   - 最后通知 UI Agent 按新枚举处理
3. 若只改 `message` 文案而不改 `code / fallback`，仍需确认不会破坏当前分类逻辑的测试
