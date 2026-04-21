# OpenKara 远端资料库计划（现状校准版）

## 摘要

本计划不再把远端资料库写成“还没开始的理想状态”，而是以仓库当前实现为准，明确：

1. **已经落地的能力**
2. **这轮实际补齐的能力**
3. **还未完成、但必须继续做完的目标**

当前目标集合固定为三个 provider：

- `google_drive`
- `dropbox`
- `webdav`

其中本轮的落地重点是：**把 WebDAV 做成真正可接入、可同步、可播放、可发布的云端资料库入口**，同时把 `google_drive` / `dropbox` 从“假接入入口”改回“明确在计划中、但尚未接线完成”的真实状态，避免产品层继续暴露伪能力。

---

## 当前代码实际进度

### A. 已完成并已在代码中存在

#### 1. 多资料库注册表与活动资料库模型

- `AppConfig` 已从单一 `library_path` 迁移到：
  - `active_library_id`
  - `libraries`
- 已支持：
  - 注册本机资料库
  - 注册远端资料库
  - 切换活动资料库
  - 移除资料库
- 旧 `library_path` 已保留升级迁移逻辑。

#### 2. 首启与设置页的远端资料库入口

- 首启 `LibrarySetup` 已有：
  - `Create new local library`
  - `Use existing local library`
  - `Open remote library`
- Settings 已能显示本机库/远端库并切换活动库。

#### 3. 远端资料库本地工作副本模型

- 远端资料库并不是“直接把所有云端文件都常驻下载到本机”，而是：
  - 在 app data 下维护一个 **cached working copy**
  - 本地仍使用 `LibraryRoot + openkara.db` 语义
  - 远端素材按需同步/下载

#### 4. 远端歌曲模型基础

- `songs.file_path` 已支持 `NULL`
- `songs.audio_source_kind` 已存在并已进库：
  - `original`
  - `original_remote`
  - `stems_remote`
- 播放服务已经具备 remote-stems 播放分流基础。

#### 5. 上传状态 UI 基础

- 已存在统一的 `TaskProgressBar`
- 全局进度条与歌曲行进度条已经能显示上传状态
- 上传状态 store / IPC 已接好

### B. 本轮已补齐到“真实可用”的范围

#### 1. WebDAV 变成真实 provider，而不是占位名词

- `RemoteLibraryProvider` 已包含 `webdav`
- 首启远端接入页已加入 **WebDAV 实际连接表单**：
  - 显示名
  - WebDAV Server URL
  - Library path
  - Username
  - Password
- 后端不再把远端认证 payload 整体忽略；WebDAV 会执行真实连接校验。

#### 2. WebDAV 远端库初始化 / 同步 / 发布 / 按需下载链路

- 连接 WebDAV 资料库时会：
  - 校验服务可达与凭据有效
  - 初始化远端根目录结构（若不存在）
  - 维护 `.openkara-library`
  - 下载或上传 `openkara.db`
- 发布到远端库时会：
  - 上传 `stems/` 或原始媒体
  - 上传更新后的 `openkara.db`
- 远端库播放时会：
  - 在本地缺少素材时按需从 WebDAV 下载

#### 3. 远端资料库产品面收口

- `google_drive` / `dropbox` 不再走假连接流程
- 首启远端页会明确区分：
  - **WebDAV：当前可用**
  - **Google Drive：代码已接通，待真实账号 smoke test**
  - **Dropbox：仍在计划中，尚未接线完成**
- 本机歌曲“分离”入口已不再对远端歌曲错误暴露。

### C. 仍未完成的目标

#### 1. Google Drive provider

- 代码已经包含真实 OAuth PKCE + loopback callback 路径
- 代码已经包含 Drive folder / file create、查找、上传、下载基础路径
- 代码已经包含远端 `openkara.db` 同步、媒体发布与按需下载分支
- 仍未完成：
  - 真实 Google 账号人工 smoke test
  - 系统 keychain 凭据落盘替换
  - 更细的冲突处理与错误文案

#### 2. Dropbox provider

- 还没有真实 OAuth / token / Dropbox file API 集成
- 还没有远端目录选择与增量同步

#### 3. 远端资料库的最终产品级收口

- 凭据仍未迁移到系统 keychain
- 远端 DB revision 冲突处理还只是最小闭环，不是完整冲突重放模型
- 远端编辑后的提示文案、失败重试、设置页“连接/解绑/删除/重连”管理流仍可继续完善

---

## Provider 目标矩阵

| Provider       | 目标状态              | 当前状态                   | 备注                                                         |
| -------------- | --------------------- | -------------------------- | ------------------------------------------------------------ |
| `webdav`       | 首个真实可用 provider | **本轮落地**               | 真实连接、同步、发布、按需下载                               |
| `google_drive` | 正式支持              | **代码已接通，待人工验证** | 已有真实 OAuth + Drive 文件路径代码，仍需真实账号 smoke test |
| `dropbox`      | 正式支持              | 计划中                     | 这轮先移除伪接入，保留为明确待完成项                         |

---

## 当前约束下的产品目标

本计划后续所有实现都必须满足下面几个目标：

1. **不能再向用户暴露假 provider 能力**
2. **远端库必须保留本地 working copy，避免 UI 全程直接耦合远端 API**
3. **远端库播放必须允许按需下载，而不是要求先全量同步**
4. **错误信息必须指出“哪一步失败、用户下一步该做什么”**
5. **archive 外只保留仍对当前或后续开发有指导价值的计划文档**

---

## 已完成项清单（代码视角）

- [x] 资料库注册表模型
- [x] 旧 `library_path` 迁移
- [x] 本机库 / 远端库统一切换
- [x] 首启远端资料库入口
- [x] 远端歌曲 `audio_source_kind`
- [x] 远端上传状态 IPC / store / UI
- [x] 远端 stems 播放分流基础
- [x] WebDAV provider 类型与前端输入表单
- [x] WebDAV 连接校验
- [x] WebDAV 远端库初始化
- [x] WebDAV `openkara.db` 上传 / 下载
- [x] WebDAV 素材发布
- [x] WebDAV 播放按需下载
- [x] 禁止继续暴露 fake Google Drive / Dropbox 连接流程

---

## 仍未完成目标的详细完成计划

### Phase 1：Google Drive provider

#### 目标

把 `google_drive` 从“计划中的 provider 名称”补成真实 provider，行为与 WebDAV 一致：

- 连接
- 选择或创建远端库根目录
- 下载 / 上传 `openkara.db`
- 发布媒体与 stems
- 按需下载远端播放素材

#### 实施步骤

1. **OAuth 基础设施**
   - 使用桌面 `Authorization Code + PKCE + loopback redirect`
   - 引入独立的 Google OAuth 配置与 token exchange 逻辑
   - 把 token 存到系统 keychain，而不是普通配置文件
2. **Drive 文件系统抽象**
   - 建立“路径 ↔ Drive file id”映射缓存
   - 固定远端根结构：
     - `.openkara-library`
     - `openkara.db`
     - `media/`
     - `media-g/`
     - `stems/`
3. **Working copy 同步器**
   - 下载最新 `openkara.db`
   - 上传变更后的 `openkara.db`
   - 支持按路径上传媒体/stems
4. **增量同步与 revision**
   - 使用 Drive revision / change token
   - 避免每次切库都全量扫描整个目录树
5. **产品侧收口**
   - 首启与设置页复用 WebDAV 的交互骨架
   - 明确“登录中 / 选择资料库 / 同步中 / 失败重试”的用户指引

#### 当前已完成到哪一步

- [x] Google OAuth client payload 与前端表单
- [x] Google PKCE authorization URL 生成
- [x] loopback callback 接收与 token exchange
- [x] access token refresh
- [x] My Drive 下按显示名创建/复用远端资料库根目录
- [x] `openkara.db` 下载 / 上传
- [x] 远端媒体 / stems 上传
- [x] 远端播放按需下载
- [ ] 真实 Google 账号 smoke test
- [ ] keychain 落盘替换
- [ ] 更细的冲突处理与错误文案

#### 验收标准

- 用户能从浏览器登录 Google
- 用户能连接已有远端库或创建新远端库
- 远端库可播放已发布歌曲
- 本机库可向绑定的 Google Drive 远端库发布歌曲

### Phase 2：Dropbox provider

#### 目标

把 `dropbox` 补齐到与 `google_drive` / `webdav` 相同的产品能力级别。

#### 实施步骤

1. 实现 Dropbox OAuth + PKCE
2. 实现 Dropbox 文件上传 / 下载 / 目录创建 / 元数据查询
3. 复用统一 remote provider 抽象，不复制 WebDAV 专属分支逻辑
4. 补齐设置页重连、断连、重新同步与错误文案

#### 验收标准

- 用户能登录 Dropbox
- 可创建 / 打开 Dropbox 远端库
- 发布、播放、同步闭环与 WebDAV 行为一致

### Phase 3：凭据与安全性收口

#### 目标

把当前可工作的凭据存储方案收口为正式发布级安全策略。

#### 实施步骤

1. 引入 **system credential store** 抽象，而不是把 macOS Keychain 写死到产品语义里：
   - macOS → Keychain
   - Windows → Credential Manager / DPAPI-backed store
   - Linux → Secret Service / libsecret（例如 GNOME Keyring / KWallet）
2. 统一凭据分层：
   - **普通配置 / registry 元数据**：不敏感字段
   - **system credential store**：refresh token / password / secret
   - **运行时内存**：当前 access token / expiry / auth session state
3. Google Drive 凭据字段按下面的最终设计拆分：
   - `config.json` / registry metadata：
     - provider
     - account_id
     - remote_root_locator
     - remote_path_display
     - display_name
     - `oauth_client_id`（不视为敏感 secret，可放普通配置）
   - system credential store：
     - `refresh_token`
     - `access_token`（可选缓存）
     - `access_token_expires_at_ms`（可选缓存）
     - `oauth_client_secret`（仅在用户提供时存这里，不进普通配置）
4. WebDAV 凭据字段按下面的最终设计拆分：
   - `config.json` / registry metadata：
     - provider
     - account_id
     - remote_root_locator
     - remote_path_display
     - display_name
     - `server_url`
   - system credential store：
     - `username`
     - `password`
5. Linux 不可用场景处理：
   - 默认先尝试 system credential store
   - 若不可用，给出明确错误并提示用户启用桌面 keyring
   - 仅在显式确认后才允许退回到本地文件 fallback
   - fallback 必须在 UI 上标记为“安全性降低”
6. 从当前本地 secrets 文件迁移历史凭据：
   - 迁移源：`remote-library-secrets.json`
   - 迁移成功后删除或清空旧 secrets 文件记录
   - 迁移失败时保留只读回退，并提示用户重连或手动迁移
7. 为迁移失败、credential store 不可用、权限拒绝、keyring daemon 未启动提供清晰错误文案

#### 验收标准

- 发布版不在普通 JSON 配置里长期保存远端凭据
- 历史用户升级后能自动迁移或得到明确修复指引

#### 当前代码与最终设计的差异

- 当前代码仍使用本地 `remote-library-secrets.json`
- 当前 Google Drive 实现会存：
  - `client_id`
  - `client_secret`（如果用户提供）
  - `access_token`
  - `refresh_token`
  - `access_token_expires_at_ms`
- 当前 WebDAV 实现会存：
  - `server_url`
  - `username`
  - `password`
- 最终发布设计不应继续把这些敏感字段长期保留在普通本地文件中

### Phase 3.5：当前未完成验证与残余风险

#### 当前未完成验证

- 没有做真实 Google 账号的手工 smoke test
  - 没实际走一遍浏览器授权
  - 没实际连真实 My Drive 做 connect / publish / play
- 没做 Playwright / UI 自动化
- 没做 Dropbox 实现

#### 当前残余风险

- 最大风险：Google Drive 代码路径已经在仓库里了，但还没有用真实 Google OAuth client + 真实账号做手工验证，所以当前状态是“代码完成并通过构建/测试”，不是“真实账户已实测通过”
- Google 凭据目前仍落在本地 secrets 文件方案，还不是 system credential store
- Rust 侧还有少量 warning，但不影响测试和构建通过

### Phase 4：远端编辑与冲突处理收口

#### 目标

把“远端库可编辑”从当前的最小同步闭环升级到真正稳健的冲突处理模型。

#### 实施步骤

1. 为 `openkara.db` 建立 provider-level revision 检查
2. 上传前若 revision 冲突：
   - 拉取最新 DB
   - 重放当前单次修改
   - 重试上传
3. 把歌词、标题、艺人、offset 修改全部纳入同一套回写管线

#### 验收标准

- 两端交替修改不会直接把 DB 覆盖坏
- 用户收到的错误信息能解释“冲突发生了什么、系统接下来做了什么”

### Phase 5：设置页与引导流产品收口

#### 目标

把当前“能接上”的远端库流程打磨到正式发布可交付状态。

#### 实施步骤

1. Settings 增加明确的远端库管理动作：
   - Connect remote library
   - Reconnect / Update credentials
   - Bind / Unbind local mirror
   - Remove remote library registration
2. 首启文案补齐：
   - provider 差异说明
   - WebDAV 输入示例
   - 常见错误定位
3. 上传失败、同步失败、认证失败的提示统一接入结构化错误

#### 验收标准

- 新用户无需查内部文档也能完成 WebDAV 接入
- 失败时能在界面上知道下一步该做什么

---

## 文档清理规则

本计划关联的文档清理规则如下：

1. `docs/plans/` 只保留仍然指导当前开发的计划
2. 过时但有追溯价值的计划移到 `docs/archive/`
3. 与当前实现冲突、且不再指导后续开发的旧计划与索引必须修正或归档

---

## 当前验收口径

本轮结束时，最低验收标准是：

- OpenKara 已能通过 **WebDAV** 接入云端资料库
- 远端库能完成：连接、注册、同步、播放、发布
- 首启远端入口不再对 Google Drive / Dropbox 暴露假完成状态
- 文档树已经把过时的一次性计划移出主路径，只留下当前与未来仍有指导意义的计划
