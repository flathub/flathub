# OpenKara 代码 Agent 执行计划

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 为负责 Rust、Tauri、SQLite、接口契约与联调的代码 Agent 提供 `Phase 1-6` 的完整执行说明，确保任何人接手都能直接继续实现。

**Architecture:** 代码 Agent 负责所有非纯展示逻辑：元数据解析、数据库、缓存、播放、推理、歌词抓取、Tauri commands/events、状态契约、CI 与发布。UI Agent 只消费稳定接口，不重新定义命令和数据结构。混合任务默认由代码 Agent 主导。

**Tech Stack:** Rust, Tauri 2, rusqlite, lofty, symphonia, cpal, tokio, reqwest, ONNX Runtime, React 19, Zustand

---

## 1. 当前起点

- `Phase 0 / M0` 已完成
- `Phase 1-4` 代码侧已完成并已冻结契约
- `Phase 5` 代码侧已完成：
  - 后端端到端 smoke flow
  - 结构化错误语义
  - 性能基线
- `Phase 6` 代码侧已部分完成：
  - Tauri build config
  - 首次启动模型 bootstrap
  - CI 模型准备
  - draft release workflow
  - 本地真实语料 smoke harness（`scripts/run-local-smoke.sh`）
- 已有基础：
  - Tauri app shell
  - SQLite migration runner
  - `songs` / `stems` / `lyrics` 表
  - Demucs 模型下载脚本和运行时 bootstrap
  - CI、本地验证命令、tag 驱动 release workflow

### 当前仍待完成的代码侧主线

1. 判断 `Phase 5.5` 是否需要 Rust/Tauri 层快捷键支持
2. 完成 `Phase 6.5` 的跨平台 smoke test 记录（本地语料脚本已就绪）
3. 完成 `Phase 6.6` Homebrew Cask 分发支持
4. 跟进首次真实 release workflow 执行结果

### 开始前先读

1. [交接总计划](./2026-03-13-handoff-master-plan.md)
2. [Technical Roadmap](../internal/roadmap.md)
3. [Development Phases](../internal/development-phases.md)

## 2. 代码 Agent 主导范围

- Rust modules
- SQLite schema 与 migration
- 文件缓存与模型缓存
- Tauri commands / events
- 前后端共享数据契约
- Zustand 中的共享应用状态契约
- 联调顺序与阶段 gate
- 构建、发布、CI、平台 smoke test

## 3. Phase 1 — Import & Library 契约

**Owner:** 代码 Agent

**Current status:** 已完成

### 任务

1. 实现 `metadata` 模块
2. 实现 `songs` 的 SQLite CRUD
3. 实现 `import_songs(paths: Vec<String>)`
4. 定义资料库读取接口和错误返回
5. 明确歌曲实体字段，供 UI Agent 消费

### 输入

- 本地音频路径
- 已存在的 `songs` 表

### 输出

- 稳定的歌曲数据结构
- 导入命令和查询命令
- 单元测试覆盖元数据解析和 SQLite 读写

### 交给 UI Agent 的契约

- `Song` 至少包含：
  - `hash`
  - `file_path`
  - `title`
  - `artist`
  - `album`
  - `duration_ms`
  - `cover_art`
- 导入结果包含成功项和失败信息的可消费结构

### 验证

```bash
cd src-tauri && cargo test
cd ..
pnpm tauri dev
```

### 建议提交边界

- `feat: add metadata parsing module`
- `feat: add songs sqlite cache`
- `feat: add import songs command`
- `docs: record phase 1 library contract`

## 4. Phase 2 — Playback Engine 与状态契约

**Owner:** 代码 Agent

**Current status:** 已完成

### 任务

1. 实现 `symphonia` 解码
2. 实现 `cpal` 播放输出
3. 实现播放状态机
4. 实现位置事件 `playback-position`
5. 定义播放器共享状态字段
6. 提供 `play / pause / seek / set_volume`

### 输出

- 可稳定播放本地音频
- 可被 UI 消费的播放状态和位置事件

### 交给 UI Agent 的契约

- 命令：
  - `play(song_id)`
  - `pause()`
  - `seek(ms)`
  - `set_volume(level)`
- 状态字段：
  - `songId`
  - `isPlaying`
  - `positionMs`
  - `volume`

### 验证

```bash
cd src-tauri && cargo test
cd ..
pnpm tauri dev
```

### 建议提交边界

- `feat: add audio decode pipeline`
- `feat: add cpal playback output`
- `feat: add playback state machine and events`
- `docs: record playback contract`

## 5. Phase 3 — Stem Separation Backend

**Owner:** 代码 Agent

**Current status:** 已完成

### 任务

1. 加载 ONNX 模型
2. 预处理 PCM 到模型输入
3. 执行推理并回写 stems
4. 混合伴奏输出
5. 实现 hash-based stems cache
6. 发出分离进度事件
7. 让分离任务后台执行，不阻塞 UI

### 输出

- 分离命令
- 进度 / 完成 / 错误事件
- 缓存命中逻辑

### 交给 UI Agent 的契约

- 事件：
  - `separation-progress`
  - `separation-complete`
  - `separation-error`
- 播放模式字段：
  - `original`
  - `karaoke`

### 验证

```bash
cd src-tauri && cargo test
cd ..
pnpm tauri build --debug --no-bundle --ci
```

### 阶段 gate

- UI Agent 只有在进度和模式契约冻结后，才能稳定实现 Phase 3 UI

### 建议提交边界

- `feat: add separation model loader`
- `feat: add separation inference pipeline`
- `feat: add stems cache and progress events`
- `feat: add background separation worker`
- `docs: record separation contract`

## 6. Phase 4 — Lyrics Backend

**Owner:** 代码 Agent

**Current status:** 已完成

### 任务

1. 实现 LRCLIB client
2. 实现 LRC parser
3. 实现抓取优先链
4. 实现歌词 SQLite cache
5. 实现 offset 读写
6. 定义歌词行数据结构和读取契约

### 输出

- 可消费的歌词行数组
- 可持久化的 offset
- 抓取命中 / miss / error 结果

### 交给 UI Agent 的契约

- 歌词行结构：
  - `time_ms`
  - `text`
- 当前歌曲 offset 字段
- 点击跳转可调用的 `seek(ms)`

### 验证

```bash
cd src-tauri && cargo test
cd ..
pnpm tauri dev
```

### 阶段 gate

- UI Agent 只有在歌词行结构与 offset 持久化语义稳定后，才能完成 Phase 4 UI 接线

### 建议提交边界

- `feat: add lrclib lyrics client`
- `feat: add lrc parser and fetch chain`
- `feat: add lyrics cache and offset persistence`
- `docs: record lyrics contract`

## 7. Phase 5 — Integration 主导项

**Owner:** 代码 Agent

**Current status:** 已完成 `5.1` 后端 smoke、`5.2` 结构化错误、`5.3` 性能基线；`5.5` 是否需要代码侧快捷键仍待确认

### 任务

1. 组织 `import -> play -> separate -> fetch lyrics -> karaoke` 端到端联调
2. 定义统一错误语义，供 UI 展示
3. 对播放延迟、歌词 jitter、seek latency 做量化验证
4. 驱动快捷键能力与焦点边界

### 与 UI Agent 的共享边界

- `5.1` 端到端联调：代码 Agent 主导
- `5.2` 错误处理逻辑：代码 Agent 主导，UI Agent 负责展示
- `5.4` UI polish：UI Agent 主导，代码 Agent 配合修接口或状态

### 输出

- 可运行的完整 Karaoke 流程
- 明确的错误类型与 fallback 语义
- 性能验证结果

### 验证

```bash
pnpm lint
pnpm format
cd src-tauri && cargo test
cd ..
pnpm tauri build --debug --no-bundle --ci
```

### 建议提交边界

- `feat: wire end-to-end karaoke flow`
- `feat: add integration error handling`
- `perf: profile playback and lyrics sync`
- `feat: add keyboard shortcut backend support`

## 8. Phase 6 — Build, Release, Distribution

**Owner:** 代码 Agent

**Current status:** 已完成 `6.1` build config、`6.2` CI 模型准备、`6.3` release workflow、`6.4` 首次启动模型 bootstrap，以及本地真实语料 smoke harness；`6.5` 的平台记录与 `6.6` Homebrew 仍待完成

### 任务

1. 完成 Tauri build config
2. 扩展 CI 到发布要求
3. 实现 GitHub Release 自动化
4. 验证首次安装模型下载
5. 做平台 smoke test
6. 编写 Homebrew Cask 并接入 tap repo

### 输出

- 跨平台构建可用
- 版本发布流程可执行
- 可分发产物

### 验证

```bash
pnpm lint
pnpm format
cd src-tauri && cargo test
cd ..
pnpm tauri build --debug --no-bundle --ci
./scripts/run-local-smoke.sh
```

### 建议提交边界

- `build: finalize tauri app configuration`
- `ci: add release workflow`
- `feat: add first-run model bootstrap in app`
- `build: add homebrew distribution support`

## 9. 共享状态与接口规则

### 默认规则

- Zustand 只要涉及跨组件共享业务状态，就归代码 Agent 主导定义
- UI Agent 可以在组件内部维护局部展示状态
- 不允许 UI 层私自扩展后端契约字段

### 接口变更要求

- 任何命令、事件、状态字段、表结构变更都必须：
  1. 先更新实现
  2. 再更新计划文档中的契约描述
  3. 最后通知 UI Agent 重新对齐

## 10. 基础验证与恢复

### 接手前执行

```bash
git status --short --branch
git log --oneline --decorate -n 10
pnpm lint
pnpm format
cd src-tauri && cargo test
cd ..
```

### 暂停前必须记录

- 当前 `Phase`
- 已冻结的契约
- UI Agent 已经依赖的字段或事件
- 尚未验证的部分
- 下一步最小动作

### 继续时先检查

- 当前分支是否仍为 `codex/phase0-m0` 或其后续工作分支
- 当前 worktree 是否正确
- 是否有未提交改动
- 自己即将做的工作是否会破坏既有 UI 契约
