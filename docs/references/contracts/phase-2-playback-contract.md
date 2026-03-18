# Phase 2 播放契约

**Goal:** 固定当前播放层命令、状态快照、stem 加载与位置事件语义，保证后续 UI 和 Karaoke 体验都基于同一套契约继续推进。

**Current starting point:** 当前实现已经把播放命令层收口为 thin Tauri command，具体编排转移到 backend playback service / CDG helper，但对外 IPC 契约保持不变。

## Owner

- 代码 Agent：播放状态机、解码、设备输出、stem 挂载、位置事件、CDG 状态
- UI Agent：消费命令返回值和位置事件，不单方面修改命令名、字段名或事件名

## Phase-by-phase task breakdown

### 已冻结能力

1. `play(song_id: String) -> PlaybackStateSnapshot`
2. `resume() -> PlaybackStateSnapshot`
3. `pause() -> PlaybackStateSnapshot`
4. `seek(ms: u64) -> PlaybackStateSnapshot`
5. `set_volume(level: f32) -> PlaybackStateSnapshot`
6. `set_stem_volume(stem: StemName, level: f32) -> PlaybackStateSnapshot`
7. `load_stems() -> PlaybackStateSnapshot`
8. `get_playback_state() -> PlaybackStateSnapshot`
9. `playback-position` 事件 payload 为 `{ ms: u64 }`

### 后续 Phase 依赖

1. UI Agent 的 `Player` 组件依赖本契约驱动 seek bar、play/pause、volume 和 stem 混音状态
2. `Phase 4` 歌词高亮将依赖 `playback-position`
3. `Phase 5` 的延迟与 jitter 验证会以本快照和事件为基线
4. `Phase 5` 起，播放命令失败值统一为 `CommandError`，详见 [phase-5-error-contract.md](./phase-5-error-contract.md)

## Inputs / outputs / required dependencies

### Command: `play`

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
  "isPlaying": true,
  "positionMs": 0,
  "durationMs": 1000,
  "volume": 1.0,
  "stemVolumes": {
    "vocals": 1.0,
    "drums": 1.0,
    "bass": 1.0,
    "other": 1.0
  },
  "hasStems": false,
  "stemMode": null
}
```

**Semantics**

1. `song_id` 对应 `songs.hash`
2. 命令会从 SQLite 读取歌曲路径，并实时解码为 `f32` PCM
3. 首次播放时会懒启动 `cpal` 输出线程
4. 如果找不到歌曲、无法解码或输出设备不可用，命令返回 `CommandError`

### Command: `pause`

**Output:** `PlaybackStateSnapshot`

**Semantics**

1. 暂停后保留当前位置
2. `isPlaying` 变为 `false`
3. 当前实现不清空已加载轨道

### Command: `resume`

**Output:** `PlaybackStateSnapshot`

**Semantics**

1. 没有已加载轨道时返回 `CommandError`
2. 恢复后从当前暂停位置继续推进
3. 若输出线程尚未启动，命令会和 `play` 一样保证输出线程已就绪

### Command: `seek`

**Input**

```json
{
  "ms": 900
}
```

**Semantics**

1. 会 clamp 到 `0..durationMs`
2. 若当前正在播放，seek 后继续播放
3. 命令完成后会立刻触发一次位置事件

### Command: `set_volume`

**Input**

```json
{
  "level": 0.35
}
```

**Semantics**

1. 取值会 clamp 到 `0.0..1.0`
2. 默认初始音量为 `1.0`
3. 音量状态独立于当前是否有已加载轨道

### Command: `set_stem_volume`

**Input**

```json
{
  "stem": "vocals",
  "level": 0.35
}
```

**Semantics**

1. 取值会 clamp 到 `0.0..1.0`
2. 目标 stem 固定为 `vocals | drums | bass | other`
3. 未加载 stems 时调用仍返回当前快照；不会隐式触发 stem 解码

### Command: `load_stems`

**Output:** `PlaybackStateSnapshot`

**Semantics**

1. 当前歌曲已挂载 stems 时，直接返回现有快照
2. 当前歌曲没有缓存 stems 时，命令返回 `CommandError`
3. stem 解码遵守 stale decode 忽略规则：如果解码完成时当前歌曲已切换，不会把 stems 附着到新歌曲

### Shared type: `PlaybackStateSnapshot`

| Field         | Type                                | Notes                     |
| ------------- | ----------------------------------- | ------------------------- |
| `songId`      | `Option<String>`                    | 当前未加载轨道时为 `null` |
| `isPlaying`   | `bool`                              | 当前是否处于播放推进状态  |
| `positionMs`  | `u64`                               | 当前播放位置              |
| `durationMs`  | `Option<u64>`                       | 未加载轨道时为 `null`     |
| `volume`      | `f32`                               | `0.0..1.0`                |
| `stemVolumes` | `{ vocals, drums, bass, other }`    | 各 stem 音量              |
| `hasStems`    | `bool`                              | 当前是否已挂载 stems      |
| `stemMode`    | `"two_stem" \| "four_stem" \| null` | 当前 stem 模式            |

### Event: `playback-position`

**Payload**

```json
{
  "ms": 1234
}
```

**Semantics**

1. 事件名固定为 `playback-position`
2. 仅在存在已加载轨道时发出
3. 后端线程约每 `16ms` 检查一次位置，并在位置变化时发出事件
4. `play`、`pause`、`seek`、`resume` 命令执行后也会立即补发一次最新位置
5. `playback-ended` 是额外内部事件，用于前端队列自动推进；不替代 `playback-position`

### Shared error type: `CommandError`

播放命令统一返回结构化错误，字段定义与错误码含义见 [phase-5-error-contract.md](./phase-5-error-contract.md)。

### Required dependencies

1. `symphonia` 负责解码支持格式
2. `cpal` 负责设备输出
3. `PlaybackController` 负责状态推进与位置计算
4. backend playback service 负责 latest-request-wins、output thread 启动和 stale decode 忽略
5. backend CDG helper 负责 sidecar / explicit path / Media+G ZIP 的 CDG 状态加载与 backward seek reset
6. `stems` cache 为 `load_stems` 提供已缓存路径

## Verification commands

```bash
cd src-tauri
cargo test
cd ..
pnpm tauri build --debug --no-bundle --ci
```

**Expected evidence**

1. `phase2_decode`
2. `phase2_playback`
3. `phase2_output`

以上测试全部通过，并且调试构建成功。

## Pause-and-resume instructions

1. 接手前先读本文件，再读 [../internal/roadmap.md](../internal/roadmap.md)
2. 先跑验证命令，确认播放层没有被后续修改打破
3. 如果后续调整命令名、字段名、事件名或节流语义：
   - 先更新本契约
   - 再改 Rust 实现
   - 最后通知 UI Agent
4. 如果进入真实设备调试阶段，请把：
   - 使用了哪些输入音频
   - 输出设备环境
   - 是否出现卡顿 / seek 偏移 / 静音

写进交接说明。
