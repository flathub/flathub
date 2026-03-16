# OpenKara UI Agent 执行计划

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 为 UI 专项 Agent 提供从 `Phase 1` 到 `Phase 6` 的完整界面工作说明，确保无需搜索其他文档即可定位全部 UI 职责。

**Architecture:** UI Agent 只负责 React/Tailwind 层的组件、交互、状态展示与视觉打磨，不定义 Rust/Tauri/SQLite 契约。所有界面任务均以代码 Agent 产出的命令、事件和状态字段为输入。混合任务默认由代码 Agent 主导。

**Tech Stack:** React 19, TypeScript 5, Vite 7, Tailwind CSS 4, Zustand, Tauri WebView

---

## 1. 当前起点

- `Phase 0 / M0` 已完成
- `Phase 1-4` 的后端契约已经就绪，可直接开始真实接线
- 现有前端基础：
  - `src/App.tsx`
  - `src/styles/globals.css`
  - `src/types/app-shell.ts`
- 当前可直接消费的契约文档：
  - [phase-1-library-contract.md](../contracts/phase-1-library-contract.md)
  - [phase-2-playback-contract.md](../contracts/phase-2-playback-contract.md)
  - [phase-3-separation-contract.md](../contracts/phase-3-separation-contract.md)
  - [phase-4-lyrics-contract.md](../contracts/phase-4-lyrics-contract.md)
  - [phase-5-error-contract.md](../contracts/phase-5-error-contract.md)
  - [phase-6-model-bootstrap-contract.md](../contracts/phase-6-model-bootstrap-contract.md)
- 后续 UI 主要落点目录：
  - `src/components/Library/`
  - `src/components/Player/`
  - `src/components/Lyrics/`
  - `src/hooks/`
  - `src/stores/`

### 开始前先读

1. [交接总计划](./2026-03-13-handoff-master-plan.md)
2. [Technical Roadmap](../internal/roadmap.md)
3. [Development Phases](../internal/development-phases.md)

## 2. UI Agent 不负责什么

- Rust 后端模块
- SQLite schema、迁移、缓存目录
- Tauri commands / events 的命名和 payload 设计
- 音频播放状态机、模型推理、歌词抓取逻辑
- 发布工作流、签名、Homebrew、平台 smoke test

## 3. Phase 1 — Music Library UI

**Owner:** UI Agent

### 任务

1. 实现 `Library` 主界面容器
2. 实现歌曲 `grid/list` 展示切换
3. 实现 `SongCard` 或等价展示单元
4. 实现 `ImportDropzone`
5. 实现文件选择入口按钮
6. 实现搜索框与筛选交互
7. 实现空态、导入中、导入失败、无结果状态

### 依赖的后端契约

- 歌曲列表数据至少包含：
  - `hash`
  - `file_path`
  - `title`
  - `artist`
  - `album`
  - `duration_ms`
  - `cover_art`
- 导入动作有明确的成功结果或错误反馈

### 输出

- 可复用的 `Library` 组件树
- 清晰的导入入口和反馈状态
- 响应式资料库列表

### 验收

- 导入 10 首歌后，列表可浏览
- 搜索关键词后，结果实时过滤
- 空库时有明确引导
- 导入错误时不出现空白页面

### 暂停恢复

- 当前优先做真实契约接线；仅在明确阻塞时才退回静态骨架
- 恢复时先确认数据字段是否变化，再继续接线

## 4. Phase 2 — Playback UI

**Owner:** UI Agent

### 任务

1. 实现 `Player` 主组件
2. 实现播放/暂停按钮
3. 实现进度条与拖动 seek
4. 实现音量滑杆
5. 实现当前播放信息展示
6. 实现播放中、暂停、加载、错误视觉状态

### 依赖的后端契约

- 播放命令：`play / pause / seek / set_volume`
- 播放状态字段：`songId / isPlaying / positionMs / volume`
- 位置更新事件：`playback-position`

### 输出

- 一套完整可交互的播放控制 UI
- 与播放状态同步的控件显示

### 验收

- 点击播放按钮能反映真实播放状态
- 拖动进度条后界面位置立即更新
- 音量操作有即时视觉反馈
- 异常状态有明确提示位

### 暂停恢复

- 若后端播放 API 未稳定，先实现静态控件和只读显示
- 恢复时优先检查事件频率和状态字段命名

## 5. Phase 3 — Separation UI

**Owner:** UI Agent

### 任务

1. 实现分离进度条或等价状态组件
2. 实现原曲 / Karaoke 模式切换控件
3. 实现缓存命中状态、处理中状态、失败状态
4. 在 Player 或 StatusBar 中整合分离状态

### 依赖的后端契约

- 分离动作触发入口
- 事件：`separation-progress / separation-complete / separation-error`
- 当前播放模式字段：`original | karaoke`

### 输出

- 明确的分离反馈路径
- 用户可理解的模式切换

### 验收

- 分离过程中进度可见
- 分离完成后可切换模式
- 错误情况下有明确信息，不阻塞其他基本操作

### 暂停恢复

- 若进度事件未稳定，先用占位状态组件，不固化假 payload

## 6. Phase 4 — Lyrics UI

**Owner:** UI Agent

### 任务

1. 实现 `Lyrics` 面板
2. 实现当前行 / 即将到来 / 已过行的视觉区分
3. 实现滚动跟随与高亮过渡
4. 实现点击歌词跳转
5. 实现偏移量控制按钮
6. 实现无歌词、抓取中、抓取失败状态

### 依赖的后端契约

- 歌词行结构：`time_ms / text`
- 当前激活行索引或可推导时间轴
- 偏移量读写能力

### 输出

- 可滚动可点击的歌词视图
- 平滑同步的视觉体验

### 验收

- 当前歌词行明显高亮
- 点击某一行后界面跳到对应时间
- 偏移调整有即时反馈且重启后仍保留
- 无歌词时有明确 fallback

### 暂停恢复

- 如果代码 Agent 尚未提供稳定歌词行结构，不提前绑定字段名

## 7. Phase 5 — Integration & Polish UI

**Owner:** UI Agent

### 任务

1. 统一 Library / Player / Separation / Lyrics 的加载态样式
2. 统一错误提示和 fallback 展示
3. 优化桌面端主视图布局
4. 优化大屏和中等窗口下的排版
5. 为快捷键相关操作补足表面反馈
6. 参与应用图标、启动图、窗口标题的视觉落地

### 共享任务

- `5.1` 端到端流程联调：代码 Agent 主导，UI Agent 配合
- `5.2` 错误处理：逻辑归代码 Agent，展示归 UI Agent

### 输出

- 一致的视觉系统
- 更稳定的异步体验
- 面向真实使用场景的桌面布局

### 验收

- 1280x800 与 1920x1080 下都可用
- 没有明显未处理的空白状态或跳闪
- 键盘操作时界面反馈清晰

### 暂停恢复

- 若共享契约变化，先等待代码 Agent 完成接口冻结

## 8. Phase 6 — 发布阶段涉及的 UI 工作

**Owner:** UI Agent（仅视觉资产部分）

### 任务

1. 如需品牌资产，提供应用 icon / splash / 品牌色修订
2. 配合验证打包后视觉是否正确

### 不负责

- release automation
- 签名
- installer 逻辑
- Homebrew

## 9. UI Agent 基础验证命令

```bash
pnpm lint
pnpm format
pnpm tauri dev
```

### 组件级验证原则

- 每个阶段至少验证：
  - 正常态
  - 加载态
  - 空态
  - 错误态
- 不用 mock 掩盖契约缺失；契约未稳定时明确标记为阻塞

## 10. 安全暂停与继续

### 暂停前必须记录

- 当前做到哪个 `Phase`
- 当前组件做到什么状态
- 尚未接上的后端契约有哪些
- 哪些界面状态已经验证，哪些还没验证

### 接手时先做

1. 运行基础验证命令
2. 阅读 [交接总计划](./2026-03-13-handoff-master-plan.md)
3. 确认代码 Agent 是否已交付本阶段契约
4. 再开始实现或联调
