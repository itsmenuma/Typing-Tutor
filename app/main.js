const { app, BrowserWindow, ipcMain } = require('electron');
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
  return new Promise((resolve, reject) => {
    execFile(
      path.join(__dirname, '../build/typingtutor.exe'),
      args,
      { cwd: path.join(__dirname, '../build') }, // <--- Set working directory!
      (error, stdout, stderr) => {
        if (error) return reject(stderr || error.message);
        resolve(stdout);
      }
    );
  });
});