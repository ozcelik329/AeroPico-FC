const { app, BrowserWindow, session, ipcMain } = require("electron");
const fs = require("fs");
const path = require("path");
const { SerialPort } = require("serialport");

let nativeSerial = null;

function serialDevicePath(name) {
  if (typeof name !== "string" || name.length === 0) return "";
  if (name.startsWith("/dev/")) return name;
  if (/^(cu|tty)\./.test(name)) return `/dev/${name}`;
  return "";
}

function openSerialPortWithTimeout(port, timeoutMs = 1500) {
  return new Promise((resolve, reject) => {
    let settled = false;
    const timer = setTimeout(() => {
      if (settled) return;
      settled = true;
      reject(new Error(`Port acma zaman asimi: ${port.path}`));
    }, timeoutMs);

    port.open((error) => {
      if (settled) {
        if (!error && port.isOpen) {
          port.close(() => {});
        }
        return;
      }
      settled = true;
      clearTimeout(timer);
      if (error) reject(error);
      else resolve();
    });
  });
}

function closeNativeSerial() {
  if (!nativeSerial) return;
  if (nativeSerial.port && nativeSerial.port.isOpen) {
    nativeSerial.port.close(() => {});
  }
  nativeSerial = null;
}

async function listNativeSerialPorts() {
  const entries = fs.readdirSync("/dev");
  return entries
    .filter((name) => /^cu\.(usb|modem|serial|wch|SLAB|debug)/i.test(name))
    .sort((a, b) => {
      const aUsb = /usbmodem/i.test(a) ? 0 : 1;
      const bUsb = /usbmodem/i.test(b) ? 0 : 1;
      return aUsb - bUsb || a.localeCompare(b);
    })
    .map((name) => ({
      portId: name,
      portName: name,
      vendorId: null,
      productId: null,
      serialNumber: null,
      displayName: `/dev/${name}`
    }));
}

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
      sandbox: false,
      webSecurity: true,
      enableBlinkFeatures: "Serial",
      preload: path.join(__dirname, "preload.js")
    }
  });

  win.loadFile(path.join(__dirname, "renderer", "index.html"));
  return win;
}

function serializePort(port) {
  const fallbackId = port.portName || port.displayName || port.serialNumber || "";
  return {
    portId: port.portId || fallbackId,
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
  let pendingPortList = [];

  function resolveSelectedPortId(requestedId) {
    if (!requestedId || pendingPortList.length === 0) return "";
    const selected = pendingPortList.find((port) => {
      return port.portId === requestedId ||
        port.portName === requestedId ||
        port.displayName === requestedId ||
        port.serialNumber === requestedId;
    });
    return selected ? selected.portId : "";
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
    pendingPortList = portList.slice();
    webContents.send("serial-port-list", portList.map(serializePort));
  });

  ipcMain.on("serial-port-choice", (_event, portId) => {
    if (pendingPortCallback) {
      pendingPortCallback(resolveSelectedPortId(portId));
      pendingPortCallback = null;
      pendingPortList = [];
    }
  });

  ipcMain.handle("native-serial-list", async () => listNativeSerialPorts());

  ipcMain.handle("native-serial-connect", async (event, { portName, baudRate }) => {
    closeNativeSerial();
    const devicePath = serialDevicePath(portName);
    if (!devicePath) throw new Error("Geçerli bir /dev/cu veya /dev/tty portu seçilmedi.");
    if (!Number.isFinite(baudRate) || baudRate <= 0) throw new Error("Geçersiz baud rate.");

    const port = new SerialPort({
      path: devicePath,
      baudRate,
      autoOpen: false,
      lock: true
    });
    await openSerialPortWithTimeout(port);
    nativeSerial = { port, path: devicePath };

    port.on("data", (chunk) => {
      event.sender.send("native-serial-data", Array.from(chunk));
    });
    port.on("error", (error) => {
      event.sender.send("native-serial-error", error.message);
    });

    return { portName: path.basename(devicePath), devicePath };
  });

  ipcMain.handle("native-serial-write", async (_event, bytes) => {
    if (!nativeSerial || !nativeSerial.port || !nativeSerial.port.isOpen) return false;
    const buffer = Buffer.from(Array.isArray(bytes) ? bytes : []);
    if (buffer.length === 0) return true;
    return new Promise((resolve) => {
      nativeSerial.port.write(buffer, (error) => {
        resolve(!error);
      });
    });
  });

  ipcMain.handle("native-serial-disconnect", async () => {
    closeNativeSerial();
    return true;
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
