// Modules to control application life and create native browser window
const { app, ipcMain, BrowserWindow, globalShortcut } = require("electron");
const path = require("path");



function createWindow() {
  // Create the browser window.
  const mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    titleBarStyle: "hidden",
    maximizable: false,
    resizable: false,
    frame: false,
    fullscreenable: false,
    webPreferences: {
      preload: path.join(__dirname, "./components/js/preload.js"),
      nodeIntegration: true,
      contextIsolation: false,
      enableRemoteModule: true,
    },
    icon: "./src/components/images/icon.png",
  });
  // and load the index.html of the app.
  mainWindow.loadFile("./src/index.html");
  // Open the DevTools.
  // mainWindow.webContents.openDevTools()
  return mainWindow;
}



// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.whenReady().then(() => {
  const bigasswindow = createWindow();
  ipcMain.on("close", () => {
    app.quit();
  });
  ipcMain.on("minimize", () => {
    bigasswindow.minimize();
  });


  app.on("browser-window-focus", function () {
    /*globalShortcut.register("CommandOrControl+R", () => {
      console.log("CommandOrControl+R is pressed: Reloading disabled");
    });*/
    globalShortcut.register("F5", () => {
      console.log("F5 is pressed: Reloading is disabled");
    });
    globalShortcut.register("CommandOrControl+Plus", () => {
      console.log("CommandOrControl+Plus: Zooming is disabled");
    });
    globalShortcut.register("CommandOrControl+-", () => {
      console.log("CommandOrControl+-: Zooming is disabled");
    });
    globalShortcut.register("CommandOrControl+Shift+R", () => {
      console.log("CommandOrControl+Shift+R: Force reloading is disabled");
    });
    globalShortcut.register("CommandOrControl+W", () => {
      console.log(
        "CommandOrControl+W is pressed: Closing using a shortcut is unsupported"
      );
    });
    /*globalShortcut.register("CommandOrControl+Shift+I", () => {
      console.log(
        "CommandOrControl+Shift+I is pressed: The development console is disabled"
      );
    });*/
  });

  app.on("browser-window-blur", function () {
    /*globalShortcut.unregister('CommandOrControl+R');*/
    globalShortcut.unregister("F5");
    globalShortcut.unregister("CommandOrControl+Plus");
    globalShortcut.unregister("CommandOrControl+-");
    globalShortcut.unregister("CommandOrControl+Shift+R");
  });

  app.on("activate", function () {
    // On macOS it's common to re-create a window in the app when the
    // dock icon is clicked and there are no other windows open.
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

// Quit when all windows are closed, except on macOS. There, it's common
// for applications and their menu bar to stay active until the user quits
// explicitly with Cmd + Q.
app.on("window-all-closed", function () {
  if (process.platform !== "darwin") app.quit();
});
