# Presentation Output Contract

**Goal:** 固定 macOS 原生 AirPlay audience 输出的前后端契约：顶部独立 AirPlay 按钮挂载系统原生 `AVRoutePickerView`，backend 通过 `AVPlayer + LAN-reachable HLS` 输出 `歌曲音频 + 歌词/CDG`，而不是把电视先当成系统显示器来选择。

## Scope

1. 本契约只覆盖 `macOS AirPlay`
2. 本地第二显示器输出仍然走现有 `fullscreen-player`
3. AirPlay 入口使用原生 `AVRoutePickerView`
4. AirPlay 媒体承载使用 backend 生成、可被接收端访问的 HLS
5. Windows 无线显示设置入口和 TV-as-display 模型不属于该契约

## Frontend behavior

1. `MonitorPicker` 只继续列出 `availableMonitors()` 返回的本地显示器
2. macOS 下，顶部独立 AirPlay 按钮挂载原生 AirPlay 控件宿主位
3. 宿主位出现时，前端调用 `sync_airplay_route_picker(bounds)`；宿主位销毁时调用 `sync_airplay_route_picker(null)`
4. 前端调用 `sync_airplay_audience_state(payload)` 同步当前歌曲/歌词配置；backend 自己负责运行时播放位置、歌词高亮和 CDG cadence
5. 纯文本歌词页通过 `step_airplay_plain_text_page({ direction })` 交给 native bridge 翻页，不再依赖主窗口滚动进度；翻页必须直接作用于当前原生渲染状态，不能通过刷新 stream generation 来换页，否则会造成可听断音
6. 当 backend 发出 `openkara://airplay-output-state` 且 `phase === "playing"`、`active === true` 时，前端关闭本地 `fullscreen-player`，避免本地第二窗口和 AirPlay 同时占用 audience 输出
7. 主窗口始终保持标准播放器 UI，不因为 AirPlay 连接状态切换成 audience 布局；只有 `fullscreen-player` 和 native AirPlay renderer 才能渲染 audience 样式

## Command: `sync_airplay_route_picker`

**Input**

```json
{
  "bounds": {
    "left": 12,
    "top": 34,
    "width": 140,
    "height": 28
  }
}
```

**Output**

```json
null
```

**Semantics**

1. `bounds` 使用 DOM 坐标系，backend 负责把它转换为 AppKit 视图坐标
2. `bounds !== null` 时，backend 把原生 `AVRoutePickerView` 挂载到当前 Tauri WebView 的 `ns_view`
3. `bounds === null` 时，backend 卸载原生 `AVRoutePickerView`
4. 该命令不打开系统设置，也不伪造系统 AirPlay 菜单

## Command: `sync_airplay_audience_state`

**Input**

```json
{
  "payload": {
    "mode": "lyrics",
    "songId": "song-1",
    "lines": [{ "time_ms": 3000, "text": "Line", "words": null }],
    "offsetMs": 100,
    "isLoading": false,
    "lyricsFontStep": 1,
    "messages": {
      "selectSong": "Select a song to start",
      "loadingLyrics": "Loading lyrics...",
      "noLyrics": "No lyrics available for this track",
      "addLyrics": "Add Lyrics"
    },
    "viewport": {
      "widthPx": 1280,
      "heightPx": 720,
      "bottomInsetPx": 0
    },
    "presentationSpec": {
      "contentWidthRatio": 0.92,
      "contentMaxWidthPx": 1600,
      "horizontalPaddingPx": 64,
      "verticalPaddingPx": 56,
      "lineGapPx": 40,
      "fontSizePx": 72,
      "lineHeightMultiple": 1.08,
      "activeScale": 1.05,
      "statusFontSizePx": 18,
      "activeGlowBlurPx": 12,
      "activeTextColor": {
        "red": 1.0,
        "green": 1.0,
        "blue": 1.0,
        "alpha": 1.0
      },
      "pastTextColor": {
        "red": 0.28235,
        "green": 0.28235,
        "blue": 0.29019,
        "alpha": 1.0
      },
      "futureTextColor": {
        "red": 0.22745,
        "green": 0.22745,
        "blue": 0.23529,
        "alpha": 1.0
      },
      "plainTextColor": { "red": 1.0, "green": 1.0, "blue": 1.0, "alpha": 1.0 },
      "statusTextColor": {
        "red": 0.55686,
        "green": 0.55686,
        "blue": 0.57647,
        "alpha": 1.0
      },
      "activeGlowColor": { "red": 1.0, "green": 1.0, "blue": 1.0, "alpha": 0.8 }
    }
  }
}
```

**Output**

```json
null
```

**Semantics**

1. `mode` 固定为 `idle | lyrics | cdg`
2. 该命令是配置同步，不是运行时节拍同步；前端只提供 `songId / lines / offsetMs / isLoading / lyricsFontStep / messages / viewport / presentationSpec`
3. backend 保存最新配置，并由自己的 coordinator 读取真实播放状态，计算当前歌词行/单词高亮和 CDG 帧，再同步到原生 AirPlay bridge
4. `messages` 提供无歌、加载中、无歌词等 audience 空状态所需的本地化文案；AirPlay 电视端只显示弱化文本提示，不再渲染按钮式空状态卡片
5. `presentationSpec` 固定了 audience 内容区宽度、字号、行距、颜色和 glow；本地 `fullscreen-player` audience 输出与 AirPlay 必须共用这套显式 spec，不能各自维护一套样式
6. `viewport` 当前固定为 `1280x720`，对应 backend HLS 视频编码参考尺寸
7. backend 从本机混音输出 tap 获取音频 PCM，并与 scene/CDG 帧一起写入 `AVAssetWriter` 分段输出，再由 `AVPlayer` 播放可被接收端访问的 HLS
8. backend 不再依赖主窗口 React 节拍驱动歌词或 CDG；AirPlay 运行时以 Rust 播放状态为唯一时间源
9. 主窗口标准 UI 仍然应跟随 `displayedPositionMs` 计算歌词和 CDG 的同步显示时钟，但不应因此切换自身布局模式
10. 该命令不修改现有播放、歌词、CDG IPC 名称，只复用它们的状态
11. backend 在写给 native bridge 的内部 JSON scene 时，使用独立 DTO 固定内层时间字段为 `timeMs`；不能直接复用共享 IPC 歌词结构，否则 nested `time_ms` 会让 native 误判 timed lyrics 为 plain text
12. backend 在 plain-text 场景下按页缓存 `pageStartIndices`，并在配置变化时重置到第一页；页内渲染由 native bridge 按当前 `pageIndex` 控制

## Command: `step_airplay_plain_text_page`

**Input**

```json
{
  "direction": "next"
}
```

**Output**

```json
null
```

**Semantics**

1. `direction` 固定为 `prev | next`
2. 该命令只对 `mode === "lyrics"` 且所有 `lines.time_ms === 0` 的 plain-text 场景生效
3. backend 先把翻页请求转发给 native bridge，由 native bridge 维护页索引和页起始位置
4. native bridge 成功切页后，backend 必须在同一次命令调用里立即同步当前 runtime，让接收端直接渲染新页；翻页不能刷新 `audio epoch` 或 `streamGeneration`
5. `mode === "idle"`、`mode === "cdg"`，或非 plain-text 歌词场景下必须直接 no-op

## Event: `openkara://airplay-output-state`

**Payload**

```json
{
  "active": true,
  "audioActive": true,
  "routeName": null,
  "mode": "lyrics",
  "phase": "playing",
  "detail": null,
  "displayedPositionMs": 1250,
  "streamGeneration": 7,
  "latencyMs": 420
}
```

**Semantics**

1. `phase` 固定为 `idle | route_selected | buffering | playing | failed`
2. `active` 只表示 audience/video 路由真正激活；只有 `phase === "playing"`、`mode !== "idle"`、`AVPlayer.currentItem` 已 ready 且 `externalPlaybackActive === true` 时为 `true`
3. `audioActive` 表示远端 AirPlay 音频路由正在实际消费 OpenKara 生成流；第三方电视与 HomePod 都可能为 `true`
4. `routeName` 当前允许为 `null`
5. `mode` 反映 backend 记录的最新 audience 输出模式
6. `detail` 提供等待/失败的简短诊断字符串；当前约定值包括 `waiting_for_route`、`waiting_for_video`、`waiting_for_audio`、`writer_failed`
7. `displayedPositionMs` 只对 audience/video 路由产生值；HomePod 这种 audio-only 路由必须保持 `null`
8. `streamGeneration` 表示当前 HLS 输出代次；seek、切歌、`lyrics <-> cdg` 切换和 plain-text 成功翻页时都必须刷新 generation，以清掉旧缓冲
9. `latencyMs` 表示 backend 当前源播放位置与 TV 显示位置的差值，用于诊断 AirPlay 实际传输延迟；audio-only 路由必须保持 `null`

## Non-goals

1. 不再保留 `open_wireless_display_settings` 或任何“先把电视连成系统显示器”的 UI/命令
2. 不新增 Chromecast、DLNA 或 Windows 无线显示入口
3. 不把当前 WebView/第二窗口直接当作 AirPlay 路由对象
4. 不使用静音 placeholder 作为实际 AirPlay 媒体源
