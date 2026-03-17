# Phase 3 分离契约

**Goal:** 固定 `Phase 3` 代码侧已经实现的分离命令、状态快照、缓存语义和事件名，保证 UI Agent 与后续代码接手者都基于同一套约定继续。

**Current starting point:** 本契约对应分支 `codex/phase0-m0` 上 `feat: add separation model loader`、`feat: add separation preprocess pipeline`、`feat: add separation inference pipeline`、`feat: add accompaniment mixing`、`feat: add stems cache`、后台 worker 接入之后的状态。

## Owner

- 代码 Agent：模型加载、推理、伴奏混音、缓存、后台任务、状态事件
- UI Agent：消费命令返回值和事件，不单方面改命令名、事件名、字段名

## 已冻结能力

1. `separate(song_id: String) -> SeparationStatusSnapshot`
2. `get_separation_status(song_id: String) -> SeparationStatusSnapshot`
3. `separation-progress` 事件 payload 为 `{ song_id: String, percent: u8 }`
4. `separation-complete` 事件 payload 为 `{ song_id: String }`
5. `separation-error` 事件 payload 为 `{ song_id: String, error: CommandError }`
6. stem cache 目录固定为 `<app_cache_dir>/stems/{song_hash}/`
7. `separate(song_id)` 只有在模型 bootstrap 为 `ready` 时才会真正启动后台 worker
8. 分离前会把输入音频归一化为 Demucs 需要的 `44.1 kHz / stereo`
9. 超过单个 Demucs window 的长音频会按固定窗口分段推理并拼回完整 stems

## Inputs / outputs / required dependencies

### Command: `separate`

**Input**

```json
{
  "songId": "sha256 hash string"
}
```

**Output**

```json
{
  "songId": "sha256 hash string",
  "state": "running",
  "percent": 0,
  "cacheHit": false,
  "vocalsPath": null,
  "accompPath": null,
  "error": null
}
```

**Semantics**

1. 如果同一首歌已经在分离中，命令直接返回现有 `running` 状态，不重复启动 worker
2. 命令本身立即返回；实际推理在后台 `spawn_blocking` worker 中执行
3. worker 会按阶段更新进度，并发出 progress / complete / error 事件
4. 如果缓存命中，后台仍会发出一次 `separation-progress`，其 `percent` 为 `100`，然后再发 `separation-complete`
5. 若运行时模型仍在下载或 bootstrap 失败，命令会直接返回 `CommandError`，不会创建任何分离任务；模型侧约束详见 [phase-6-model-bootstrap-contract.md](./phase-6-model-bootstrap-contract.md)

### Command: `get_separation_status`

**Input**

```json
{
  "songId": "sha256 hash string"
}
```

**Semantics**

1. 如果该歌曲还没有任何分离记录，返回 `idle` 状态
2. `completed` 状态会带上 `vocalsPath` 和 `accompPath`
3. `failed` 状态会带上结构化错误 `CommandError`

### Shared type: `SeparationStatusSnapshot`

| Field        | Type                                             | Notes                          |
| ------------ | ------------------------------------------------ | ------------------------------ |
| `songId`     | `String`                                         | 对应 `songs.hash`              |
| `state`      | `"idle" \| "running" \| "completed" \| "failed"` | 状态字段固定为 snake_case enum |
| `percent`    | `u8`                                             | `0..100`                       |
| `cacheHit`   | `bool`                                           | 仅 `completed` 时可能为 `true` |
| `vocalsPath` | `Option<String>`                                 | `completed` 时存在             |
| `accompPath` | `Option<String>`                                 | `completed` 时存在             |
| `error`      | `Option<CommandError>`                           | `failed` 时存在                |

### Events

#### `separation-progress`

```json
{
  "songId": "sha256 hash string",
  "percent": 70
}
```

#### `separation-complete`

```json
{
  "songId": "sha256 hash string"
}
```

#### `separation-error`

```json
{
  "songId": "sha256 hash string",
  "error": {
    "code": "separation_failed",
    "message": "failed to separate stems for song song-a",
    "retryable": true,
    "fallback": "retry"
  }
}
```

### Shared error type: `CommandError`

分离失败状态和 `separation-error` 事件统一复用结构化错误，字段定义与错误码含义见 [phase-5-error-contract.md](./phase-5-error-contract.md)。

## Cache semantics

1. 完整 stem 输出会写进 `<app_cache_dir>/stems/{song_hash}/`
2. 目录内至少有：
   - `vocals.wav`
   - `accompaniment.wav`
3. SQLite `stems` 表记录：
   - `song_hash`
   - `vocals_path`
   - `accomp_path`
   - `separated_at`
4. 如果数据库记录存在但文件丢失，后端会重新生成并覆盖目录

## Required dependencies

1. `symphonia` 负责解码输入音频
2. `ort` 负责 Demucs ONNX 推理
3. `rubato` 负责把非 `44.1 kHz` 输入重采样到 Demucs 目标采样率
4. `vorbis_rs` 负责 stem / accompaniment OGG 写盘
5. `tauri::async_runtime::spawn_blocking` 负责后台执行推理任务

## Verification commands

```bash
cd src-tauri
cargo test
cd ..
pnpm tauri build --debug --no-bundle --ci
```

**Expected evidence**

1. `phase3_model`
2. `phase3_preprocess`
3. `phase3_inference`
4. `phase3_mix`
5. `phase3_stems_cache`
6. `phase3_job`
7. `phase3_status`

以上测试全部通过，并且调试构建成功。

## Pause-and-resume instructions

1. 接手前先读本文件，再读 [../../exec-plans/active/2026-03-13-code-agent-plan.md](../../exec-plans/active/2026-03-13-code-agent-plan.md)
2. 先跑验证命令，确认分离链路没有被后续改动打破
3. 如果要改命令名、事件名、状态字段或缓存目录：
   - 先更新本契约
   - 再改 Rust 实现
   - 最后通知 UI Agent
4. 下一阶段推荐顺序：
   - 接 `set_playback_mode`，把 `original / karaoke` 切换接到播放层
   - 再进入歌词后端 `Phase 4`
