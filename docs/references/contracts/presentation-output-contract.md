# Presentation Output Contract

**Goal:** 固定 macOS 原生 AirPlay audience 输出的前后端契约：顶部独立 AirPlay 按钮挂载系统原生 `AVRoutePickerView`，backend 通过 `AVPlayer + loopback HLS` 输出 `歌曲音频 + 歌词/CDG`，而不是把电视先当成系统显示器来选择。

## Scope

1. 本契约只覆盖 `macOS AirPlay`
2. 本地第二显示器输出仍然走现有 `fullscreen-player`
3. AirPlay 入口使用原生 `AVRoutePickerView`
4. AirPlay 媒体承载使用 backend 生成的 loopback HLS
5. Windows 无线显示设置入口和 TV-as-display 模型不属于该契约

## Frontend behavior

1. `MonitorPicker` 只继续列出 `availableMonitors()` 返回的本地显示器
2. macOS 下，顶部独立 AirPlay 按钮挂载原生 AirPlay 控件宿主位
3. 宿主位出现时，前端调用 `sync_airplay_route_picker(bounds)`；宿主位销毁时调用 `sync_airplay_route_picker(null)`
4. 前端持续调用 `sync_airplay_audience_state(payload)`，把当前 audience scene 同步到 backend
5. 当 backend 发出 `openkara://airplay-output-state` 且 `active === true` 时，前端关闭本地 `fullscreen-player`，避免本地第二窗口和 AirPlay 同时占用 audience 输出

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
    "isPlaying": true,
    "positionMs": 3210,
    "lines": [{ "time_ms": 3000, "text": "Line", "words": null }],
    "activeLineIndex": 0,
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
2. backend 保存最新 audience 输出状态，并把当前 mode、歌词 scene 或 CDG 帧同步到原生 AirPlay bridge
3. `messages` 提供无歌、加载中、无歌词等 audience 空状态所需的本地化文案
4. `viewport` 当前固定为 `1280x720`，对应 backend HLS 视频编码参考尺寸
5. backend 从本机混音输出 tap 获取音频 PCM，并与 scene/CDG 帧一起写入 `AVAssetWriter` 分段输出，再由 `AVPlayer` 播放 loopback HLS
6. 该命令不修改现有播放、歌词、CDG IPC 名称，只复用它们的状态

## Event: `openkara://airplay-output-state`

**Payload**

```json
{
  "active": true,
  "routeName": null,
  "mode": "lyrics"
}
```

**Semantics**

1. `active` 表示原生 AirPlay route 已建立，并且真实媒体 item 已经附着到 `AVPlayer`
2. `routeName` 当前允许为 `null`
3. `mode` 反映 backend 记录的最新 audience 输出模式

## Non-goals

1. 不再保留 `open_wireless_display_settings` 或任何“先把电视连成系统显示器”的 UI/命令
2. 不新增 Chromecast、DLNA 或 Windows 无线显示入口
3. 不把当前 WebView/第二窗口直接当作 AirPlay 路由对象
4. 不使用静音 placeholder 作为实际 AirPlay 媒体源
