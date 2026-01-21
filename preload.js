const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("electron", {
  receive: (channel, callback) => {
    const validChannels = ["play-media", "load-addon", "unload-addon"];
    if (validChannels.includes(channel)) {
      ipcRenderer.removeAllListeners(channel); // Prevent multiple callbacks
      ipcRenderer.on(channel, (_event, ...args) => callback(...args));
    }
  },
});
