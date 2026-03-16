# OpenKara 交接总计划

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 为后续接手的 UI Agent 和代码 Agent 提供唯一入口文档，确保任何时刻中断后都能继续推进 `Phase 1-6`。

**Architecture:** 当前仓库已完成 `Phase 0 / M0`，后续工作按现有阶段文档推进。执行上分为两条主线：代码 Agent 负责 Rust、Tauri、SQLite、命令事件、状态契约与联调主导；UI Agent 负责 React/Tailwind 组件、交互、视觉状态和界面打磨。`Phase 3` 与 `Phase 4` 在 `Phase 2` 之后允许并行。

**Tech Stack:** Tauri 2, Rust, rusqlite, React 19, TypeScript 5, Vite 7, Tailwind CSS 4, Zustand

---

## 1. 当前起点

### 仓库与工作区

- 当前开发分支：`codex/phase0-m0`
- 当前工作树：`/Users/david/Documents/Vibe/OpenKara/.worktrees/phase0-m0`
- 上游主分支：`main`

### 已完成阶段

- `Phase 0 / M0` 已完成
- `Phase 1 / M1` 代码侧已完成，UI 未开始
- `Phase 2 / M2` 代码侧已完成，UI 未开始
- `Phase 3 / M3` 代码侧已完成，UI 未开始
- `Phase 4 / M4` 代码侧已完成，UI 未开始
- `Phase 5` 代码侧已完成：
  - 端到端后端 smoke flow
  - 结构化错误语义
  - 后端性能基线
- `Phase 6` 代码侧部分完成：
  - Tauri build config
  - 首次启动模型 bootstrap
  - CI 模型准备
  - tag 驱动 release workflow
- 当前仓库具备：
  - 可运行的 Tauri 桌面壳
  - React + TypeScript + Vite 前端基础
  - Tailwind CSS 4、ESLint、Prettier
  - SQLite migration、`songs` / `stems` / `lyrics` 缓存
  - 本地导入、播放、分离、歌词后端主链
  - Demucs 模型脚本 + 首次启动运行时下载
  - CI 与 draft release workflow

### 关键提交

- `0e36d11` `perf: add backend performance baseline`
- `7c6d824` `docs: add phase 5 performance baseline`
- `c2c71ca` `feat: add first-run model bootstrap`
- `75bc782` `docs: record model bootstrap contract`
- `f49d7be` `build: finalize tauri app configuration`
- `024dec4` `ci: align verification with model setup`
- `6a241d9` `ci: add release workflow`
- `d924214` `docs: expand install and release instructions`

### 接手前必须先读

1. 本文档
2. [UI Agent 执行计划](./2026-03-13-ui-agent-plan.md)
3. [代码 Agent 执行计划](./2026-03-13-code-agent-plan.md)
4. [Development Phases](../internal/development-phases.md)
5. [Technical Roadmap](../internal/roadmap.md)

## 2. 阶段总览与依赖

### 主顺序

1. `Phase 1` 音乐导入与资料库
2. `Phase 2` 原始音频播放
3. `Phase 3` AI 人声分离
4. `Phase 4` 同步歌词
5. `Phase 5` 集成与打磨
6. `Phase 6` 构建、发布与分发

### 并行规则

- `Phase 0` 和 `Phase 1` 已经是顺序关系，不能跳过
- `Phase 2` 完成之后，`Phase 3` 和 `Phase 4` 可以并行
- `Phase 5` 必须等待 `Phase 1-4` 全部可用
- `Phase 6` 必须等待 `Phase 5` 联调收口完成

### 阶段门槛

- `Phase 1` 完成门槛：本地歌曲能导入、持久化、展示
- `Phase 2` 完成门槛：原始音频可播放、暂停、拖动、调音量
- `Phase 3` 完成门槛：伴奏分离可缓存、可显示进度、可切换模式
- `Phase 4` 完成门槛：歌词可抓取、显示、同步、偏移调整
- `Phase 5` 完成门槛：端到端 Karaoke 流程稳定
- `Phase 6` 完成门槛：CI 构建稳定、可发布二进制

## 3. 责任划分

### UI Agent 负责

- `Library`、`ImportDropzone`、`Player`、`Lyrics`、`StatusBar`、`SeparationProgress`
- 所有加载态、空态、错误态、响应式布局、键盘交互表面、视觉 polish
- 对既有后端契约的消费与 React/Tailwind 实现

### 代码 Agent 负责

- Rust 模块：`metadata`、`cache`、`audio`、`separator`、`lyrics`、`commands`
- SQLite 表结构、缓存目录策略、模型加载、网络请求、播放状态机
- Tauri commands / events / setup、状态契约、联调主导
- 构建发布、CI、Homebrew、平台 smoke test

### 混合任务默认规则

- 混合任务默认 **代码 Agent 主导，UI Agent 配合**
- UI Agent 不单独发明新的后端命令、事件、表字段、持久化语义
- 如需调整接口，先由代码 Agent 更新契约，再由 UI Agent 消费

## 4. Phase 1-6 接力方式

### Phase 1

- 代码 Agent 先完成：元数据读取、SQLite CRUD、`import_songs`
- UI Agent 随后完成：资料库页面、导入交互、搜索筛选

### Phase 2

- 代码 Agent 先完成：音频解码、输出、状态机、播放位置事件
- UI Agent 随后完成：Player 控件、进度条、音量、播放态显示

### Phase 3

- 代码 Agent 主导：模型加载、推理、缓存、进度事件、后台任务
- UI Agent 跟进：分离进度、模式切换、分离结果状态展示

### Phase 4

- 代码 Agent 主导：LRCLIB、LRC 解析、缓存、偏移持久化
- UI Agent 跟进：歌词面板、滚动高亮、点击跳转、偏移控制

### Phase 5

- 两条线汇合
- 代码 Agent 主导端到端联调、错误处理逻辑与性能收口
- UI Agent 主导视觉 polish、异步体验统一、快捷键表面体验

### Phase 6

- 代码 Agent 主导全部发布与平台验证
- UI Agent 仅在需要时参与品牌视觉资产与 splash/icon 落地

## 5. Hand-off Protocol

### 开始前

1. 执行 `git status --short --branch`
2. 执行 `git log --oneline --decorate -n 10`
3. 阅读本文档和自己职责对应的计划文档
4. 根据职责运行基础验证

### 基础验证命令

```bash
pnpm lint
pnpm format
cd src-tauri && cargo test
cd ..
pnpm tauri build --debug --no-bundle --ci
```

### 执行中

- 每完成一个可独立审查的子系统就提交一次
- 提交信息应明确对应阶段和职责
- 如实现改变了契约，必须同时更新相关计划文档或交接注记

### 安全暂停

- 停止前必须满足：
  - `git status` 可解释
  - 当前分支和 worktree 路径已记录
  - 未提交改动的目的和下一步动作写入交接说明或本地注记
- 若无法整理到干净状态，至少要补一段“当前做到哪里 / 下一步是什么 / 什么还没验证”

## 6. Resume Checklist

### 新接手的人先检查

```bash
git status --short --branch
git log --oneline --decorate -n 10
pnpm lint
pnpm format
cd src-tauri && cargo test
cd ..
```

### 然后确认

- 自己是 UI Agent 还是代码 Agent
- 当前处于哪个 `Phase`
- 所需前置契约是否已经落地
- 是否存在未提交改动

### 最后再开始实现

- UI Agent：从 [UI Agent 执行计划](./2026-03-13-ui-agent-plan.md) 中找到当前阶段任务
- 代码 Agent：从 [代码 Agent 执行计划](./2026-03-13-code-agent-plan.md) 中找到当前阶段任务

## 7. 当前建议的继续顺序

### 代码 Agent 下一步

1. 评估 `Phase 5.5` 键盘快捷键是否仍需 Rust/Tauri 侧支持；若需要，先冻结快捷键契约
2. 完成 `Phase 6.5` 平台 smoke test 记录
3. 完成 `Phase 6.6` Homebrew 分发支持
4. 若 release workflow 首次实际运行暴露问题，以 workflow 与打包配置修复为最高优先级

### UI Agent 下一步

1. 直接从 `Phase 1` `Library` 页面开始
2. 优先消费已冻结契约：
   - [phase-1-library-contract.md](../contracts/phase-1-library-contract.md)
   - [phase-2-playback-contract.md](../contracts/phase-2-playback-contract.md)
   - [phase-3-separation-contract.md](../contracts/phase-3-separation-contract.md)
   - [phase-4-lyrics-contract.md](../contracts/phase-4-lyrics-contract.md)
3. 先做真实数据接线，再补视觉 polish
4. `Phase 5` 里只负责表面体验，不改后端命令和事件

## 8. 中断恢复原则

- 不以口头上下文为前提
- 以仓库里的计划文档、提交历史和验证命令为唯一恢复依据
- 如果文档与代码不一致，以代码真实状态为准，并立即修正文档
- 任何接手者都应优先减少不确定性，而不是继续堆功能
