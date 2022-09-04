// Modules to control application life and create native browser window
const { app, BrowserWindow, ipcMain, globalShortcut } = require("electron");
const path = require("path");
const DiscordRPC = require("discord-rpc");

function createWindow() {
  // Create the browser window.
  const mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    titleBarStyle: "hidden",
    maximizable: false,
    resizable: false,
    fullscreenable: false,
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
      nodeIntegration: true,
      contextIsolation: false,
      enableRemoteModule: true,
    },
    icon: "src/images/icon.png",
  });

  // and load the index.html of the app.
  mainWindow.loadFile("index.html");
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

  const clientId = "1012098922652631062";
  const rpc = new DiscordRPC.Client({ transport: "ipc" });
  const startTimestamp = new Date();

  async function setActivity() {
    rpc.setActivity({
      details: "Installing AvdanOS....",
      state: "Formatting HDD.......",
      startTimestamp,
      largeImageKey: "defaultpfp",
      largeImageText: "Installing AvdanOS",
      instance: false,
    });
  }

  rpc.on("ready", () => {
    setActivity();
    setInterval(() => {
      setActivity();
    }, 15000);
  });

  rpc.login({ clientId }).catch(console.error);

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

// In this file you can include the rest of your app's specific main process
// code. You can also put them in separate files and require them here.
