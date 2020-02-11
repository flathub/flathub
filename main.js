import { app, BrowserWindow } from "electron"

function createWindow() {
  // Create the browser window.
  win = new BrowserWindow({
    show: false,
    backgroundColor: '#04060c',
    icon: 'build/icon.png',
    webPreferences: {
      nodeIntegration: true
    },

  })
  win.maximize()

  // if (app.isPackaged || process.env.PRODUCTION) {
  //   // and load the index.html of the app.
  //   win.loadFile("dist/index.html")
  // } else {
  //   // webpack-dev-server defaults to port 8080
  //   const port = process.env.PORT || 8080;
  //   // win.loadURL(`http://localhost:${port}`)
  win.loadURL('https://bolls-256717.appspot.com')
  // }
  win.once('ready-to-show', () => {
    win.show()
  })
  win.setMenuBarVisibility(false)
}

app.on("ready", createWindow)

// Make OSX work same as all other systems
app.on("window-all-closed", () => {
  app.quit()
})

app.on("activate", () => {
  // On macOS it"s common to re-create a window in the app when the
  // dock icon is clicked and there are no other windows open.
  if (win === null) {
    createWindow()
  }
})

global.sharedObject = {
  argv: process.argv
}
