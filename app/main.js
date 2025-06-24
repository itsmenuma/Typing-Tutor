const { app, BrowserWindow, ipcMain, dialog } = require('electron');
const { execFile } = require('child_process');
const path = require('path');

function createWindow () {
  const win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false
    }
  });
  win.loadFile('index.html');
}

app.whenReady().then(createWindow);

ipcMain.handle('run-typing-tutor', async (event, args) => {
  return new Promise((resolve) => {
    execFile(
      path.join(__dirname, '../build/typingtutor.exe'),
      args,
      { cwd: path.join(__dirname, '../build') },
      (error, stdout, stderr) => {
        if (error) {
          resolve(`ERROR: ${stderr || error.message}`);
        } else {
          resolve(stdout);
        }
      }
    );
  });
});

//IPC for exiting the app gracefully in case of crash
ipcMain.on('quit-app', () => {
  app.quit();
});

