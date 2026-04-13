# Phase 6 模型 Bootstrap 契约

**Goal:** 固定运行时模型解析、首次启动下载、状态查询和分离前置条件，避免 UI Agent 和后续代码接手者继续把“模型在哪里、什么时候可用”留成隐式约定。

**Current starting point:** 本契约对应分支 `codex/phase0-m0` 上首次启动模型 bootstrap、运行时状态快照和分离前置 gate 已接入之后的状态。

## Owner

- 代码 Agent：模型路径解析、下载、校验、状态快照、事件、分离前置条件
- UI Agent：消费状态命令和事件，不单方面改命令名、事件名、字段名

## 已冻结能力

1. 应用启动时优先检查 `<app_data_dir>/models/htdemucs.onnx`
2. 若运行时安装目录缺失模型，则回退检查开发目录 `src-tauri/models/htdemucs.onnx`
3. 若两处都没有可验证的模型，应用启动后会在后台下载模型到 `<app_data_dir>/models/`
4. 应用启动时会先显式加载 `src-tauri/generated/onnxruntime/` 或已打包资源中的
   ONNX Runtime 1.23.2 动态库
5. 分离会复用同一份动态库初始化，不再依赖 `ort`/pyke 自动下载预编译运行时
6. `get_model_bootstrap_status() -> ModelBootstrapStatusSnapshot`
7. `separate(song_id)` 在模型未 ready 时立即返回 `CommandError`
8. 事件：
   - `model-bootstrap-progress`
   - `model-bootstrap-ready`
   - `model-bootstrap-error`

## 开发仓库与运行时分发规则

1. 开发仓库中的 `src-tauri/models/` 只保留 `.gitkeep` 与说明文档；下载得到的
   `.onnx` 文件必须保持为本地忽略文件，不进入 git 历史
2. `scripts/setup.sh` 只用于本地开发、离线验证或需要稳定模型输入的测试
3. `scripts/prepare-onnx-runtime.mjs` 负责把官方 ONNX Runtime CPU 1.23.2
   动态库下载并规整到 `src-tauri/generated/onnxruntime/`
4. 面向终端用户时，默认安装位置是 `<app_data_dir>/models/`，不是仓库目录；
   但打包产物会随应用一起分发 ONNX Runtime 动态库
5. 后续如果调整模型来源、运行时库版本或文件名，必须同时更新：
   - 本契约
   - `scripts/setup.sh`
   - `scripts/prepare-onnx-runtime.mjs`
   - `src-tauri/models/README.md`
6. 当前 pinned 的 release 资源为：
   - `htdemucs`: `model-v2.0.1/htdemucs.onnx`
   - `htdemucs_ft`: `model-ft-v2.0.1/htdemucs_ft.onnx`
7. `openkara-models v2.0.1` 资源会携带：
   - `openkara.model_cache_key`
   - `openkara.optimized_by=onnxruntime`
     Rust 运行时必须把前者纳入 session cache key 失效条件，并对后者关闭重复图优化。

## Inputs / outputs / required dependencies

### Command: `get_model_bootstrap_status`

**Output**

```json
{
  "state": "downloading",
  "modelPath": "/Users/example/Library/Application Support/com.openkara.desktop/models/htdemucs.onnx",
  "downloadedBytes": 1048576,
  "totalBytes": 52428800,
  "error": null
}
```

### Shared type: `ModelBootstrapStatusSnapshot`

| Field             | Type                                                              | Notes                                                                                                                         |
| ----------------- | ----------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| `state`           | `"pending" \| "downloading" \| "outdated" \| "ready" \| "failed"` | 状态字段固定为 snake_case enum；`outdated` 表示托管路径上存在文件但 SHA-256 与当前 pin 不一致（文件保留，供用户在设置中删除） |
| `modelPath`       | `String`                                                          | 当前运行时实际模型路径或目标安装路径                                                                                          |
| `downloadedBytes` | `Option<u64>`                                                     | `downloading` 时存在                                                                                                          |
| `totalBytes`      | `Option<u64>`                                                     | 下载端若返回 `Content-Length` 则存在                                                                                          |
| `error`           | `Option<CommandError>`                                            | `failed` 时存在                                                                                                               |

### Events

#### `model-bootstrap-progress`

payload 为完整的 `ModelBootstrapStatusSnapshot`，其中：

- `state = "downloading"`：`downloadedBytes` 在事件流中应单调不减（实现侧可节流）；`model_path` 固定为运行时安装路径
- `state = "outdated"`：校验失败但文件未自动删除；`downloadedBytes` / `totalBytes` 为 `null`
- `state = "pending"`：等待后台 worker 或用户操作

#### `model-bootstrap-ready`

payload 为完整的 `ModelBootstrapStatusSnapshot`，其中：

- `state = "ready"`
- `downloadedBytes = null`
- `error = null`

#### `model-bootstrap-error`

payload 为完整的 `ModelBootstrapStatusSnapshot`，其中：

- `state = "failed"`
- `error.code = "model_unavailable"`
- `error.fallback = "retry"`

## Runtime path resolution semantics

1. 优先使用活动模型 variant 对应的 `<app_data_dir>/models/<descriptor.filename>`
2. 若运行时安装目录已有模型且 SHA-256 校验通过，直接进入 `ready`
3. 若运行时安装目录模型存在但校验失败（含旧版本 pin 不匹配），进入 `outdated`，**保留**托管文件以便用户在设置的危险区删除；不会静默删除后再下载
4. 若运行时安装目录缺失，但开发目录 `src-tauri/models/<descriptor.filename>` 存在且校验通过，则直接进入 `ready`
5. 只有当两处都没有可用模型时，才会在后台从固定 URL 下载到运行时安装目录

## ONNX Runtime path resolution semantics

1. 应用启动时先检查打包后的资源目录 `onnxruntime/<platform-lib>`
2. macOS 打包产物额外允许从 `Contents/Frameworks/libonnxruntime.dylib` 解析
3. 若未打包运行时，则回退到开发目录 `src-tauri/generated/onnxruntime/<platform-lib>`
4. 若三处都找不到运行时动态库，应用启动立即失败并提示执行
   `./scripts/setup.sh` 或 `node scripts/prepare-onnx-runtime.mjs`
5. Rust 侧固定使用 `ort 2.0.0-rc.11`，对应官方 ONNX Runtime CPU 1.23.2
   动态库；不允许重新打开 `download-binaries` 或 `copy-dylibs`
6. macOS 发布产物必须按目标架构分别准备 `arm64` / `x86_64` 动态库，不允许再把
   universal2 ORT 放进两个安装包里浪费体积

## Product UX target

现有后端行为已经支持“启动后自动 bootstrap + 状态事件 + 分离前置 gate”。后续
UI 与产品行为应以以下目标为准，而不是把后台下载继续当成隐式行为：

1. 启动时检查模型是否存在且校验通过
2. 若缺失，提示模型大小、安装位置和用途，并提供：
   - `Download now`
   - `Later`
3. 用户选择下载后，后台执行下载并显示真实进度
4. 用户选择稍后时，资料库和原曲播放仍然可用，但分离与 Karaoke 模式保持禁用
5. 当用户首次进入 Karaoke 或主动触发分离时，如模型仍未 ready，需要再次提示
6. 下载失败时，UI 使用现有 `model-bootstrap-error` 状态提供重试入口，而不是要求用户手动找脚本

## Separation gate semantics

1. `separate(song_id)` 现在依赖模型 bootstrap 状态
2. `state = ready` 时才允许启动推理 worker
3. `state = pending / downloading / outdated / failed` 时，命令立即返回（`outdated` 时错误文案应引导用户打开设置删除旧文件并重新下载）：

```json
{
  "code": "model_unavailable",
  "message": "model bootstrap is still downloading to ...",
  "retryable": true,
  "fallback": "retry"
}
```

4. 该前置条件不会修改 `get_separation_status(song_id)` 的语义；状态查询仍只反映分离任务自身状态

## Required dependencies

1. `reqwest` 负责运行时模型下载
2. `sha2` 负责 SHA-256 完整性校验
3. `tauri::async_runtime::spawn_blocking` 负责后台下载，避免阻塞 app setup
4. `ort 2.0.0-rc.11` 仅以 `load-dynamic` 模式加载官方 ONNX Runtime 1.23.2

## Verification commands

```bash
cd src-tauri
cargo test --test phase6_model_bootstrap
cargo test
cd ..
node scripts/prepare-onnx-runtime.mjs
pnpm tauri build --debug --no-bundle --ci
```

**Expected evidence**

1. `phase6_model_bootstrap` 证明路径解析、已验证写盘、状态 gate 正常
2. 全量 `cargo test` 证明现有分离/播放/歌词链路未被打破
3. 调试构建成功

## Pause-and-resume instructions

1. 接手前先读本文件，再读 [../internal/roadmap.md](../internal/roadmap.md)
2. 若需要更换模型 URL、校验值或安装目录：
   - 先更新本契约
   - 再改 Rust 实现和测试
   - 最后通知 UI Agent
3. 若后续要给 UI 暴露下载重试按钮，优先在此契约基础上新增 `retry_model_bootstrap()`，不要改现有状态字段
