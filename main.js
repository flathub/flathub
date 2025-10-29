const { app, BrowserWindow, ipcMain, shell } = require('electron');
const path = require('path');
const contextMenu = require('electron-context-menu');

let mainWindow;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1200,
    height: 800,
    autoHideMenuBar: true,
    frame: true,
    webPreferences: {
      nodeIntegration: true,
      webviewTag: true,
      nodeIntegrationInSubFrames: true,
      preload: path.join(__dirname, 'preload.js'),
    },
  });

  mainWindow.loadURL('https://www.icloud.com/mail/', {userAgent: 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36'});

  mainWindow.on('closed', function () {
    mainWindow = null;
  });

  mainWindow.webContents.once('did-finish-load', () => {
    // Check notification permission when the window is ready
    checkNotificationPermission();
    setupExternalLinkHandling();
    sendNotification();
  });
}

function setupExternalLinkHandling() {
  // Open external links in the default browser
  mainWindow.webContents.on('will-navigate', (event, url) => {
    if (isExternalLink(url)) {
      event.preventDefault();
      shell.openExternal(url);
    }
  });
}

function isExternalLink(url) {
  // Check if the URL is from iCloud.com or its subdomains
  const icloudDomains = ['icloud.com', 'apple.com', /* Add more if needed */];
  return !icloudDomains.some(domain => url.includes(domain));
}

function checkNotificationPermission() {
  // Implement notification permission logic here if needed
}

function sendNotification() {
  // Send an IPC message to show a notification
  mainWindow.webContents.send('show-notification', {
    title: 'iCloud Notification',
    body: 'This is a sample notification from iCloud!',
  });
}

app.on('ready', createWindow);

// Enable context menus
app.whenReady().then(() => {
  // Enable context menus
  contextMenu({
    showInspectElement: false, // Hide "Inspect Element" option
    prepend: (params, browserWindow) => [
      {
        label: 'Copy',
        role: 'copy',
      },
      {
        label: 'Cut',
        role: 'cut'
      },
      {
        label: 'Paste',
        role: 'paste',
      },
      { type: 'separator' },
      {
        label: 'Undo',
        role: 'undo'
      },
      {
        label: 'Redo',
        role: 'redo'
      },
      { type: 'separator' },
      {
        label: 'Zoom In',
        role: 'zoomIn'
      },
      {
        label: 'Zoom Out',
        role: 'zoomOut'
      },
      {
        label: 'Reset Zoom',
        role: 'resetZoom'
      },
      { type: 'separator' },
      {
        label: 'Reload',
        role: 'forceReload'
      },
      {
        label: 'Toggle Full Screen',
        role: 'togglefullscreen'
      }
    ],
  });
})

app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') app.quit();
});

app.on('activate', function () {
  if (mainWindow === null) createWindow();
});
