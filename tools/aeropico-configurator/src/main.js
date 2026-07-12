const { app, BrowserWindow, session, ipcMain } = require("electron");
const path = require("path");

function createWindow() {
  const win = new BrowserWindow({
    width: 1240,
    height: 820,
    minWidth: 980,
    minHeight: 680,
    title: "AeroPico Configurator",
    backgroundColor: "#0d1620",
    webPreferences: {
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true,
      webSecurity: true,
      enableBlinkFeatures: "Serial",
      preload: path.join(__dirname, "preload.js")
    }
  });

  win.loadFile(path.join(__dirname, "renderer", "index.html"));
  return win;
}

function serializePort(port) {
  return {
    portId: port.portId,
    portName: port.portName || port.displayName || null,
    vendorId: port.vendorId || null,
    productId: port.productId || null,
    serialNumber: port.serialNumber || null,
    displayName: port.displayName || null
  };
}

app.whenReady().then(() => {
  // Holds the pending Chromium callback while the renderer shows a manual
  // port picker. Only one connect flow is expected at a time.
  let pendingPortCallback = null;

  session.defaultSession.setPermissionCheckHandler((webContents, permission) => {
    const url = webContents && typeof webContents.getURL === "function" ? webContents.getURL() : "";
    return permission === "serial" && url.startsWith("file://");
  });

  session.defaultSession.setDevicePermissionHandler(({ deviceType }) => {
    return deviceType === "serial";
  });

  session.defaultSession.on("select-serial-port", (event, portList, webContents, callback) => {
    event.preventDefault();

    if (portList.length === 0) {
      callback("");
      return;
    }

    // Resolve any stale pending request before starting a new one.
    if (pendingPortCallback) {
      pendingPortCallback("");
      pendingPortCallback = null;
    }

    pendingPortCallback = callback;
    webContents.send("serial-port-list", portList.map(serializePort));
  });

  ipcMain.on("serial-port-choice", (_event, portId) => {
    if (pendingPortCallback) {
      pendingPortCallback(portId || "");
      pendingPortCallback = null;
    }
  });

  app.on("web-contents-created", (_event, contents) => {
    contents.setWindowOpenHandler(() => ({ action: "deny" }));
  });

  createWindow();

  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") app.quit();
});
