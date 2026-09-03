const { app, BrowserWindow, Menu, session, ipcMain } = require("electron");
const path = require("path");

function createWindow() {
  const win = new BrowserWindow({
    width: 1240,
    height: 820,
    minWidth: 860,
    minHeight: 600,
    title: "AeroPico Configurator",
    autoHideMenuBar: true,
    backgroundColor: "#0d1620",
    show: false,
    webPreferences: {
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true,
      webSecurity: true,
      enableBlinkFeatures: "Serial",
      preload: path.join(__dirname, "preload.js")
    }
  });

  win.setMenuBarVisibility(false);
  win.setAutoHideMenuBar(true);
  win.maximize();
  win.once("ready-to-show", () => {
    win.show();
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
    displayName: port.displayName || null,
    deviceInstanceId: port.deviceInstanceId || null
  };
}

function isAeroPicoPort(port) {
  const vendorId = String(port.vendorId || "").toLowerCase().replace(/^0x/, "");
  const identity = `${port.portName || ""} ${port.displayName || ""}`;
  return vendorId === "2e8a" || /aeropico|pico 2|rp2350/i.test(identity);
}

app.whenReady().then(() => {
  Menu.setApplicationMenu(null);

  // Holds the pending Chromium callback while the renderer shows a manual
  // port picker. Only one connect flow is expected at a time.
  let pendingPortCallback = null;
  let pendingPortWebContents = null;
  let pendingPorts = new Map();

  function clearPendingPortRequest() {
    pendingPortCallback = null;
    pendingPortWebContents = null;
    pendingPorts = new Map();
  }

  function publishPendingPorts() {
    if (pendingPortCallback && pendingPortWebContents && !pendingPortWebContents.isDestroyed()) {
      pendingPortWebContents.send("serial-port-list", [...pendingPorts.values()].map(serializePort));
    }
  }

  session.defaultSession.setPermissionCheckHandler((webContents, permission) => {
    const url = webContents && typeof webContents.getURL === "function" ? webContents.getURL() : "";
    return permission === "serial" && url.startsWith("file://");
  });

  session.defaultSession.setDevicePermissionHandler(({ deviceType }) => {
    return deviceType === "serial";
  });

  session.defaultSession.on("select-serial-port", (event, portList, webContents, callback) => {
    event.preventDefault();

    // Resolve any stale pending request before starting a new one.
    if (pendingPortCallback) {
      pendingPortCallback("");
      clearPendingPortRequest();
    }

    if (portList.length === 0) {
      callback("");
      return;
    }

    const aeroPicoPorts = portList.filter(isAeroPicoPort);
    if (aeroPicoPorts.length === 1) {
      webContents.send("serial-port-auto-selected", serializePort(aeroPicoPorts[0]));
      callback(aeroPicoPorts[0].portId);
      return;
    }

    pendingPortCallback = callback;
    pendingPortWebContents = webContents;
    pendingPorts = new Map(portList.map((port) => [port.portId, port]));
    publishPendingPorts();
  });

  session.defaultSession.on("serial-port-added", (_event, port, webContents) => {
    if (!pendingPortCallback || webContents !== pendingPortWebContents) return;
    pendingPorts.set(port.portId, port);
    publishPendingPorts();
  });

  session.defaultSession.on("serial-port-removed", (_event, port, webContents) => {
    if (!pendingPortCallback || webContents !== pendingPortWebContents) return;
    pendingPorts.delete(port.portId);
    publishPendingPorts();
  });

  ipcMain.on("serial-port-choice", (event, portId) => {
    if (!pendingPortCallback || event.sender !== pendingPortWebContents) return;
    const selectedPortId = portId && pendingPorts.has(portId) ? portId : "";
    if (portId && !selectedPortId && !event.sender.isDestroyed()) {
      event.sender.send("serial-port-selection-error", "Seçilen port artık mevcut değil.");
    }
    pendingPortCallback(selectedPortId);
    clearPendingPortRequest();
  });

  ipcMain.on("serial-port-cancel", (event) => {
    if (!pendingPortCallback || event.sender !== pendingPortWebContents) return;
    pendingPortCallback("");
    clearPendingPortRequest();
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
