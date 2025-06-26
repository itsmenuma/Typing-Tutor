const { app, BrowserWindow, ipcMain, dialog } = require('electron');
const { execFile } = require('child_process');
const path = require('path');
const fs = require('fs');

function createWindow() {
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
      { cwd: path.join(__dirname, '../build') },
      (error, stdout, stderr) => {
        if (error) return reject(stderr || error.message);
        resolve(stdout);
      }
    );
  });
});

// IPC for exporting stats
ipcMain.handle('export-stats', async (event, statsArray) => {
  const win = BrowserWindow.getFocusedWindow();

  const { filePath, canceled } = await dialog.showSaveDialog(win, {
    title: 'Save Typing Stats',
    defaultPath: 'typing-stats',
    filters: [
      { name: 'Text File', extensions: ['txt'] },
      { name: 'CSV File', extensions: ['csv'] }
    ]
  });

  if (canceled || !filePath) return false;

  // Determine file type
  const isCSV = filePath.endsWith('.csv');

  const content = isCSV
    ? 'Label,Value\n' + statsArray.map(stat => `${stat.label},${stat.value}`).join('\n')
    : statsArray.map(stat => `${stat.label}: ${stat.value}`).join('\n');

  fs.writeFileSync(filePath, content, 'utf-8');
  return true;
});
