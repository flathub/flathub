# OpenKara 多资料库、混合远端资料库与上传进度统一化计划

## 摘要

将 OpenKara 从“单一本地资料库路径”升级为“资料库注册表 + 活动资料库”模型，支持像 Calibre 一样在同一台电脑上切换多个本机资料库和远端资料库。  
首版远端资料库固定支持 `Google Drive` 与 `Dropbox`，并采用“混合远端库”模型：

- 对界面上有“分离”按钮的普通歌曲：永不上传原曲，只上传分轨文件。
- 对 `CDG`、标记为伴奏的曲目、以及其他当前不会出现“分离”按钮的文件：允许上传原始云端资产，并在远端库中直接可播。
- 每个资料库在云端和本地都维护自己独立的一份 `openkara.db`，互不共享。

旧 `config.json` 对新版本保持**升级兼容**：新版本会自动把旧的单 `library_path` 配置迁移到新注册表模型；迁移后不保证旧版 OpenKara 还能读取新配置。  
首启无资料库时，开箱流程必须新增“直接打开远端资料库”入口。  
分离进度与上传进度统一用同一个进度条组件渲染，只通过文案区分。

## 关键实现变更

### 1. 资料库注册表、切库能力与首启远端入口

- `AppConfig` 从单一 `library_path` 改为：
  - `active_library_id: string | null`
  - `libraries: RegisteredLibrary[]`
- `RegisteredLibrary` 固定为：
  - `local`: `{ id, kind, display_name, root_path }`
  - `remote`: `{ id, kind, display_name, provider, remote_root_id, account_id, cached_db_path, remote_revision }`
- `AppState` 不再只持有一个 `Option<LibraryRoot>`，改为持有 `ActiveLibraryContext`：
  - `LocalLibraryContext`
  - `RemoteLibraryContext`
- 启动逻辑改为：
  - 无注册资料库：进入 Library Manager / 首次设置
  - 有活动资料库且可打开：直接进入应用
  - 活动资料库不可用：进入资料库选择器，不直接回退到“无库”
- 首启 `LibrarySetup` 保留现有本机选项，并新增第三个选项：
  - `Create new local library`
  - `Use existing local library`
  - `Open remote library`
- `Open remote library` 首启流程固定为：
  1. 选择 provider
  2. OAuth 登录
  3. 选择一个 OpenKara 管理的远端资料库目录
  4. 下载该库的 `openkara.db` 到本地缓存
  5. 设为活动资料库
  6. 继续进入 stem mode / 语言后的剩余首次设置流程
- 应用内新增正式的资料库管理器：
  - Settings 增加 `Manage Libraries…`
  - App 菜单增加 `Switch Library…`
  - 支持：创建本机库、注册本机库、连接远端库、切换活动库、移除注册项
- 切库必须原子重置运行态：
  - 停止播放
  - 清空 player / lyrics / queue / separation / upload / library stores
  - 重新 hydrate 目标资料库的 songs / status / settings

### 2. 远端资料库改为“混合远端库”

- 每个远端资料库在云端拥有独立根目录，固定布局：
  - `.openkara-library`
  - `openkara.db`
  - `media/`
  - `media-g/`
  - `stems/`
- 上传规则固定为两类：
  - `可分离普通歌曲`：只上传 `stems/`，绝不上传 `media/` 原曲
  - `不可分离歌曲`：上传原始资产到 `media/` 或 `media-g/`
- 远端资料库展示范围固定为：
  - 已上传 stems 的普通歌曲
  - 已上传原始资产的 `CDG` / `instrumental` / 其他不可分离文件
  - 不展示未发布到云端的普通原曲
- 远端歌曲模型固定分为：
  - `original_remote`：云端有原始资产，可直接按原曲模式播放
  - `stems_remote`：云端只有 stems，播放时直接进入 karaoke/stems 模式
- `paired` CDG 与 `zip` Media+G 在远端继续保留原始媒体模型，不转成 stems-only 抽象
- 本机完整资料库与远端资料库依旧是两个独立库，只通过 `song_hash` 对齐歌曲身份

### 3. 数据模型、发布状态与播放分流

- `songs` 表新增：
  - `audio_source_kind TEXT NOT NULL DEFAULT 'original'`
- `songs.file_path` 改为可空：
  - 本机库与远端原曲型歌曲：非空
  - 远端 stems-only 歌曲：空
- `songs.audio_source_kind` 固定值：
  - `original`
  - `original_remote`
  - `stems_remote`
- 本机库新增一张本地发布状态表，用于跟踪“某首歌对某个远端资料库的发布状态”，至少记录：
  - `remote_library_id`
  - `song_hash`
  - `asset_mode` (`stems_only` / `original_asset`)
  - `published_at`
  - `remote_revision`
  - `dirty_metadata`
- 后端 `play(song_id)` 统一分流：
  - `local + original`：维持当前行为，先播原曲，再按缓存决定是否挂 stems
  - `remote + original_remote`：先确保远端原始资产下载到本地缓存，再按原曲路径播放
  - `remote + stems_remote`：先确保远端 stems 下载到本地缓存，再直接以 stems 模式启动播放，返回 `has_stems = true`
- `load_stems()`：
  - 对远端 `stems_remote` 歌曲做幂等短路返回
  - 对远端 `original_remote` 与本机歌曲继续按当前缓存语义工作
- 远端资料库能力位固定暴露给前端：
  - `can_import`
  - `can_separate`
  - `can_extract_embedded_metadata`
  - `can_publish_remote`
  - `is_remote`
- 前端基于“活动资料库能力 + 歌曲能力”控制入口：
  - 远端库隐藏 Import、Separate、Batch Separate、Extract Embedded Lyrics/Cover
  - 本机库保留这些入口
  - 本机库在已绑定远端库时显示 `Publish to Remote Library`

### 4. 自动上传、手动发布与远端数据库回写

- v1 provider 固定为：
  - `google_drive`
  - `dropbox`
- OAuth 使用桌面 `Authorization Code + PKCE + loopback redirect`
- token 存系统 keychain，不存资料库数据库，不存普通 `config.json`
- 一个本机完整资料库在 v1 最多关联一个远端资料库镜像
- 上传策略固定为双轨：
  - `普通可分离歌曲`：分轨完成后自动进入上传队列
  - `不可分离歌曲`：通过显式 `Publish to Remote Library` 动作发布，可单曲或批量
- 自动上传/手动发布的统一执行顺序固定为：
  1. 根据歌曲能力计算 `asset_mode`
  2. 上传 `stems/` 或原始资产
  3. 在远端库本地工作副本中 upsert `songs / stems / lyrics`
  4. 比对 provider revision
  5. 上传新的 `openkara.db`
  6. 回写本机发布状态表
- 远端库上的歌词、标题、艺人修改回写规则固定为：
  - 远端库内的修改直接落本地缓存 db
  - 随后回写云端 `openkara.db`
  - revision 冲突时重新下载最新 db，在新副本上重放当前单个修改，再重试上传
- 网络延迟优化固定包含：
  - 资料库列表只依赖本地缓存 db
  - 当前曲预下载
  - 下一首预取
  - 下载/上传使用 provider 的 resumable / ranged 能力
  - provider 变更同步使用增量 cursor / token，不做全量重扫

### 5. 分离进度与上传进度统一 UI

- 新增统一的 `TaskProgressBar` 组件，从现有 `GlobalProgressBar` 的条形 UI 抽出，固定输入：
  - `label`
  - `detail`
  - `percent`
  - `indeterminate`
  - `onCancel?`
- 现有分离进度和新增上传进度必须复用同一个组件，只通过文案区分：
  - `Separating: <song title>`
  - `Uploading to remote library: <song title>`
- 进度展示位置固定为两层：
  - 全局底部 `GlobalProgressBar`：显示所有活动任务，允许同屏出现分离和上传两条
  - 歌曲行 `SongListItem`：若该歌曲正在分离或上传，显示同风格小型进度条；若同一歌曲两种任务同时存在，则纵向堆叠两条
- 上传进度状态新增独立 IPC 与前端 store，不复用 separation 状态：
  - `UploadStatusSnapshot`
  - `upload-progress`
  - `upload-complete`
  - `upload-error`
  - `get_all_upload_statuses()`
- `library-store` 新增：
  - `uploadStatuses: Record<string, UploadStatusSnapshot>`
  - `updateUploadStatus`
  - `clearAllUploadStatuses`
- `GlobalProgressBar` 的任务顺序固定为：
  1. model download
  2. batch separation
  3. single-song separation
  4. single-song upload
- 上传完成后的状态不保留常驻进度条；只在运行中和失败时展示

## Breaking Changes 与兼容方案

本次共有 **6 类 breaking changes**。其中 **1 类直接影响老 `config.json`**，**2 类直接影响 SQLite / IPC 契约**，其余是运行时行为 breaking change。  
结论：**老配置文件对新版本保持升级兼容，不会导致旧用户升级后无法启动；但迁移后的新配置不保证旧版 OpenKara 继续可读。**

1. `AppConfig` 从单一 `library_path` 升级为资料库注册表，这是配置文件 breaking change。  
   解决方案：保留旧字段 `library_path` 作为**只读迁移输入**；`load_config()` 先尝试读取新结构，缺失时再读取旧字段并在内存中生成一个 local library 注册项。首次成功保存时回写新结构，之后不再把 `library_path` 当真源。

2. 启动与设置页依赖 `get_library_path()` 的逻辑会失效，这是 IPC breaking change。  
   解决方案：同一变更内一次性替换为 `get_library_registry()` / `get_active_library()` / `switch_library()`；不保留长期双接口。所有 startup gating、LibrarySetup、SettingsOverlay 同步切换。

3. `songs.file_path` 需要从 `NOT NULL` 变为可空，这是数据库 schema breaking change。  
   解决方案：新增数据库迁移，重建 `songs` 表而不是用空字符串兼容；迁移时完整复制旧数据，并为现有歌曲填入 `audio_source_kind='original'`。旧本机资料库升级后可直接继续使用。

4. `Song` IPC 结构会新增 `audio_source_kind`，且 `file_path` 变成 `string | null`，这是前后端类型 breaking change。  
   解决方案：同步修改 Rust struct、`src/types/ipc.ts`、所有读取 `song.file_path` 的前端逻辑与测试；不接受运行时兜底猜测。

5. 播放语义不再等价于“先播原曲，再可选挂 stems”；远端 `stems_remote` 歌曲会直接以 stems 模式启动播放，这是行为 breaking change。  
   解决方案：更新 Phase 2 / Phase 3 contract，明确 `play()` 可直接返回已带 stems 的 snapshot，`load_stems()` 在该场景做幂等返回；同步更新 player workflow 测试。

6. 远端存储模型从原计划的 stems-only 固定化为“混合远端库”，这是远端目录与发布资格的 breaking change。  
   解决方案：首版就把远端根目录定为 `media/ + media-g/ + stems/ + openkara.db`，避免二次改模。上传资格统一通过后端谓词决定：
   - `canSeparate == true`：只允许发布 stems
   - `canSeparate == false`：允许发布原始资产

## 公开接口与契约变更

- 废弃单库 IPC：
  - `get_library_path()`
  - `create_library(path)`
  - `open_library(path)`
- 新 IPC：
  - `get_library_registry()`
  - `create_local_library(parent_dir, display_name)`
  - `register_local_library(path, display_name?)`
  - `connect_remote_library(provider, display_name?)`
  - `switch_library(library_id)`
  - `remove_library(library_id)`
  - `get_active_library()`
  - `sync_active_remote_library()`
  - `publish_song_to_remote(song_id)`
  - `publish_songs_to_remote(song_ids)`
  - `get_all_upload_statuses()`
- 新事件：
  - `upload-progress`
  - `upload-complete`
  - `upload-error`
- `Song` IPC 变更：
  - `file_path: string | null`
  - `audio_source_kind: "original" | "original_remote" | "stems_remote"`
- 需要同步更新的契约文档：
  - Phase 1 library contract
  - Phase 2 playback contract
  - Phase 3 separation contract
  - error contract
  - onboarding / product spec

## 测试与验收

- 配置迁移：
  - 旧 `config.json` 只有 `library_path` 时，升级后自动生成单个 local library 注册项并成功启动
  - 新版本保存后，不再依赖 `library_path`
  - 迁移失败时给出明确错误并进入资料库选择器，而不是空白启动
- 首启流程：
  - 无任何资料库时，首启页面同时出现本机新建、本机打开、远端打开三个入口
  - 远端打开流程可在不创建本机资料库的前提下直接完成首启
- 多资料库切换：
  - 本机库 A / 本机库 B / 远端库之间切换都能完整刷新状态
  - 切库时当前播放停止，旧状态不泄漏
- SQLite 迁移：
  - 旧本机资料库升级后 `songs.file_path` 完整保留
  - 新增 `audio_source_kind='original'`
  - 现有导入、搜索、播放、分离、歌词流程不回归
- 远端发布：
  - 普通可分离歌曲分轨完成后自动上传 stems，不上传原曲
  - CDG / instrumental / 其他不可分离文件能手动发布并进入远端库
  - 远端库可播放已发布的 CDG 与伴奏曲目
- 远端播放：
  - `original_remote` 曲目能下载原始资产并正常播放
  - `stems_remote` 曲目能直接以 stems 模式播放
  - Media+G ZIP 与 paired CDG 在远端库路径下仍能正确解码音频和图形
- 上传进度 UI：
  - 分离进度与上传进度使用同一组件渲染
  - 全局区能同时显示两类任务
  - 歌曲行能显示当前歌曲的上传或分离进度
  - 上传失败时有重试入口，文案与分离失败保持同风格
- 远端 db 回写：
  - 修改远端库歌词、标题、艺人后可回写云端数据库
  - revision 冲突时能自动重拉并重试当前变更

## 默认假设

- v1 只做 `Google Drive + Dropbox`，不包含 `MEGA`
- “不用分离的文件”按当前产品语义定义为：界面上不会出现“分离”按钮的曲目
- 远端资料库内：
  - 普通可分离歌曲以 `stems_remote` 形式存在
  - `CDG` / `instrumental` / 其他不可分离文件以 `original_remote` 形式存在
- 配置兼容目标仅包含“旧配置升级到新版本自动迁移”，不包含“新配置继续兼容旧版 OpenKara”
- 不保留长期双真源；迁移完成后，资料库注册表是唯一配置真源
