# Presentation Output Contract

**Goal:** 固定 macOS 原生 AirPlay 入口的最小前后端契约，让 `MonitorPicker` 可以挂载系统原生 `AVRoutePickerView`，并同步 AirPlay 输出状态，而不是把电视先当成系统显示器来选择。

## Scope

1. 本契约只覆盖 `macOS AirPlay`
2. 本地第二显示器输出仍然走现有 `fullscreen-player`
3. AirPlay 入口使用原生 `AVRoutePickerView`
4. Windows 无线显示设置入口和 TV-as-display 模型不属于该契约

## Frontend behavior

1. `MonitorPicker` 继续列出 `availableMonitors()` 返回的本地显示器
2. 在 macOS 下，`MonitorPicker` 额外挂载一个原生 AirPlay 控件宿主位
3. 宿主位出现时，前端调用 `sync_airplay_route_picker(bounds)`；菜单关闭时调用 `sync_airplay_route_picker(null)`
4. 前端持续调用 `sync_airplay_audience_state(payload)`，把当前观众输出模式同步到 backend
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
    "lyricsFontStep": 1
  }
}
```

**Output**

```json
null
```

**Semantics**

1. `mode` 固定为 `idle | lyrics | cdg`
2. backend 保存最新 audience 输出状态，并把当前模式同步到原生 AirPlay bridge
3. 当前实现会驱动原生 AirPlay 路由状态和本地输出互斥控制
4. 该命令已经预留歌词/CDG audience 渲染所需的数据字段，但不修改现有播放、歌词、CDG IPC 名称

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

1. `active` 表示原生 AirPlay route 当前是否处于外放状态
2. `routeName` 当前允许为 `null`
3. `mode` 反映 backend 记录的最新 audience 输出模式

## Non-goals

1. 不再保留 `open_wireless_display_settings` 或任何“先把电视连成系统显示器”的 UI/命令
2. 不新增 Chromecast、DLNA 或 Windows 无线显示入口
3. 不把当前 WebView/第二窗口直接当作 AirPlay 路由对象
